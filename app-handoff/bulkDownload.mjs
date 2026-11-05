// Adapter contract:
// await link.subscribe(uuid, onBytes, onError) -> unsubscribe function;
//   resolves ONLY once native isNotifying is true. onBytes receives Uint8Array.
// await link.write(uuid, Uint8Array) -> native Write With Response completed.
// await link.command(cmd, fields) -> decoded semantic result from existing v2 manager.
// command adapter owns request IDs and existing JSON fragmentation.
export const DATA_UUID = '607d9703-6764-4cc9-b134-70e3c04c8500';
export const ACK_UUID = '607d9704-6764-4cc9-b134-70e3c04c8500';
const u32 = (b, at) => new DataView(b.buffer, b.byteOffset, b.byteLength).getUint32(at, true);
const u16 = (b, at) => b[at] | (b[at + 1] << 8);
const magic = (b, s) => [...s].every((c, i) => b[i] === c.charCodeAt(0));
export function crc32(bytes, initial = 0xffffffff) {
  let crc = initial;
  for (const value of bytes) {
    crc ^= value;
    for (let bit = 0; bit < 8; ++bit) crc = (crc >>> 1) ^ ((crc & 1) ? 0xedb88320 : 0);
  }
  return crc >>> 0; // caller applies final XOR exactly once
}
export function ackFrame(transferId, nextOffset, credits = 16) {
  const bytes = new Uint8Array(16);
  bytes.set([65, 66, 75, 49]);
  const view = new DataView(bytes.buffer);
  view.setUint32(4, transferId, true); view.setUint32(8, nextOffset, true);
  view.setUint16(12, credits, true);
  return bytes;
}
export class BulkReceiver {
  constructor(open, manifest, bootId, credits = 16) {
    for (const key of ['jobId', 'captureId', 'fileId', 'sensorId', 'byteLength', 'sampleCount', 'crc32'])
      if (open[key] !== manifest[key]) throw new Error(`Bulk manifest mismatch: ${key}`);
    if (open.bootId !== bootId || !Number.isInteger(open.transferId) || open.transferId < 1 || open.transferId > 0xffffffff)
      throw new Error('Stale boot or invalid transfer');
    if (!Number.isInteger(manifest.byteLength) || manifest.byteLength < 46 || manifest.byteLength > 640044 ||
        !/^[0-9a-f]{8}$/.test(manifest.crc32) || !Number.isInteger(open.mtu) ||
        open.valueBytes !== Math.min(open.mtu - 3, 512) || open.valueBytes <= 20 ||
        !Number.isInteger(credits) || credits < 1 || credits > 16)
      throw new Error('Invalid bulk limits');
    this.open = open; this.manifest = manifest; this.credits = credits;
    this.bytes = new Uint8Array(manifest.byteLength);
    this.next = 0; this.inWindow = 0; this.notifications = 0; this.crc = 0xffffffff;
    this.awaitingGrant = false;
  }
  accept(packet) {
    // Late packets from a cancelled handle must not contaminate a new receiver.
    if (packet.length >= 8 && magic(packet, 'ABW1') && u32(packet, 4) !== this.open.transferId) return null;
    if (this.awaitingGrant) throw new Error('Packet beyond granted window');
    if (packet.length < 21 || packet.length > this.open.valueBytes || !magic(packet, 'ABW1'))
      throw new Error('Invalid bulk packet');
    const offset = u32(packet, 8), count = u16(packet, 14);
    if (packet[12] !== this.manifest.sensorId || offset !== this.next ||
        packet.length !== 20 + count || offset + count > this.bytes.length ||
        packet[13] !== (offset + count === this.bytes.length ? 1 : 0) ||
        packet.subarray(16, 20).some(byte => byte !== 0))
      throw new Error('Bulk gap, duplicate, identity or length mismatch');
    const data = packet.subarray(20);
    this.bytes.set(data, offset); this.crc = crc32(data, this.crc);
    this.next += count; ++this.inWindow; ++this.notifications;
    const final = this.next === this.bytes.length;
    if (final && ((this.crc ^ 0xffffffff) >>> 0).toString(16).padStart(8, '0') !== this.manifest.crc32)
      throw new Error('Bulk CRC mismatch');
    const acknowledge = final || this.inWindow === this.credits;
    if (acknowledge) { this.inWindow = 0; this.awaitingGrant = true; }
    return { offset: this.next, acknowledge, final };
  }
  grant() { this.awaitingGrant = false; }
}

const clock = () => globalThis.performance?.now() ?? Date.now();
// One file only. The app retains READ_FILE fallback and owns verified local storage.
// validateWav is mandatory and must check format, rate, sample count and RIFF lengths.
export async function downloadBulk(link, capabilities, manifest, options) {
  if (capabilities.bulkDownload?.version !== 1 || typeof options?.validateWav !== 'function')
    throw new Error('Bulk unavailable or WAV validator missing');
  const bootId = capabilities.bootId;
  let open, receiver, unsubscribe, timer, settled = false, failure;
  let ackChain = Promise.resolve();
  let resolveData, rejectData;
  const done = new Promise((resolve, reject) => { resolveData = resolve; rejectData = reject; });
  // Avoid an unhandled rejection while subscribe/open is still awaiting native I/O.
  done.catch(() => {});
  const fail = error => {
    if (settled) return;
    settled = true; failure = error instanceof Error ? error : new Error(String(error));
    clearTimeout(timer); rejectData(failure);
  };
  const arm = () => {
    clearTimeout(timer);
    timer = setTimeout(() => fail(new Error('Bulk window stalled')), capabilities.bulkDownload.ackTimeoutMs ?? 10000);
  };
  const abort = () => fail(new Error('Bulk download cancelled by user'));
  const begun = clock();
  try {
    if (options.signal?.aborted) throw new Error('Bulk download cancelled by user');
    options.signal?.addEventListener('abort', abort);
    unsubscribe = await link.subscribe(DATA_UUID, packet => {
      if (settled || !receiver) return;
      try {
        const event = receiver.accept(packet);
        if (!event) return;
        arm(); options.onProgress?.(receiver.next, manifest.byteLength);
        if (event.acknowledge) {
          ackChain = ackChain.then(async () => {
            if (failure) throw failure;
            if (event.final) await options.validateWav(receiver.bytes, manifest);
            if (failure) throw failure;
            // Allow RX immediately: the next window may arrive before native ACK
            // write completion. Calls remain serialized on ackChain.
            receiver.grant();
            await link.write(ACK_UUID, ackFrame(open.transferId, event.offset));
            if (event.final && !settled) {
              settled = true; clearTimeout(timer); resolveData();
            }
          });
          ackChain.catch(fail);
        }
      } catch (error) { fail(error); }
    }, fail);
    if (failure) throw failure;
    open = await link.command('BULK_OPEN', { bootId, fileId: manifest.fileId, sensorId: manifest.sensorId });
    if (failure) throw failure;
    receiver = new BulkReceiver(open, manifest, bootId);
    arm();
    await link.write(ACK_UUID, ackFrame(open.transferId, 0));
    await done;
    if (failure) throw failure;
    // Native ACK completion can precede firmware queue processing. Poll status,
    // without repeating OPEN or capture, until final ACK is semantically observed.
    let status;
    for (let attempt = 0; attempt < 10; ++attempt) {
      if (options.signal?.aborted) throw new Error('Bulk download cancelled by user');
      status = await link.command('BULK_STATUS', { bootId, transferId: open.transferId });
      if (status.bootId !== bootId || status.transferId !== open.transferId || status.fileId !== manifest.fileId)
        throw new Error('Stale bulk status');
      if (status.state === 'complete') break;
      if (status.state === 'failed' || status.state === 'cancelled') throw new Error(status.error || status.state);
      await new Promise(resolve => setTimeout(resolve, 100));
    }
    if (status?.state !== 'complete' || status.acknowledgedBytes !== manifest.byteLength)
      throw new Error('Final bulk ACK unconfirmed');
    const elapsedMs = clock() - begun;
    return { bytes: receiver.bytes, metrics: { elapsedMs, bytesPerSecond: manifest.byteLength * 1000 / elapsedMs,
      mtu: open.mtu, payloadBytes: open.payloadBytes, notifications: receiver.notifications,
      firmware: status } };
  } catch (error) {
    fail(error);
    let terminalBulkStatus = null;
    if (open) {
      try { terminalBulkStatus = await link.command('BULK_CANCEL', { bootId, transferId: open.transferId }); } catch { /* link may be retired */ }
      try { terminalBulkStatus = await link.command('BULK_STATUS', { bootId, transferId: open.transferId }); } catch { /* preserve cancel snapshot */ }
    }
    try {
      await options.onFailure?.({ bootId, jobId: manifest.jobId, fileId: manifest.fileId,
        sensorId: manifest.sensorId, transferId: open?.transferId ?? null,
        acceptedBytes: receiver?.next ?? 0, nextOffset: receiver?.next ?? 0,
        error: String(error), terminalBulkStatus,
        timing: terminalBulkStatus?.timing ?? null });
    } catch { /* diagnostic sink must not replace the original transfer error */ }
    throw error;
  } finally {
    settled = true; clearTimeout(timer);
    options.signal?.removeEventListener('abort', abort);
    unsubscribe?.();
  }
}

import { downloadBulk } from './bulkDownload.mjs';
const now = () => globalThis.performance?.now() ?? Date.now();
const sleep = ms => new Promise(resolve => setTimeout(resolve, ms));

export function verifyScanManifest(result, bootId, jobId) {
  if (result.bootId !== bootId || result.jobId !== jobId || result.phase !== 'complete' || result.files?.length !== 8)
    throw new Error('Scan is not an eight-file completed job');
  const captures = new Set(), files = new Set();
  result.files.forEach((file, index) => {
    if (file.jobId !== jobId || file.sensorId !== index + 1 ||
        typeof file.captureId !== 'string' || typeof file.fileId !== 'string' ||
        !/^[a-zA-Z0-9_-]{1,40}$/.test(file.captureId) || !/^[a-zA-Z0-9_-]{1,40}$/.test(file.fileId) ||
        captures.has(file.captureId) || files.has(file.fileId) ||
        !Number.isInteger(file.sampleCount) || file.sampleCount < 1 || file.sampleCount > 320000 ||
        file.byteLength !== 44 + file.sampleCount * 2 || !/^[0-9a-f]{8}$/.test(file.crc32))
      throw new Error(`Invalid finalized manifest at Sensor ${index + 1}`);
    captures.add(file.captureId); files.add(file.fileId);
  });
  // CRCs are per-file measurements. Identical audio can legitimately have equal CRCs.
  return result.files;
}

// Reference physical runner, not enabled automatically in the existing Mac app.
// persistAndReadback must save the original bytes to the owning patient/session,
// verify byte-for-byte readback and return the local playable URI.
export async function runEightSensorBenchmark(link, capabilities, options) {
  if (capabilities.diagnostics?.build !== 'v2-bulk-timing-3' || !capabilities.ready ||
      capabilities.bulkDownload?.version !== 1 || !capabilities.modes?.includes('scan'))
    throw new Error('The accepted baseline is not ready');
  if (typeof options?.persistAndReadback !== 'function' || typeof options.validateWav !== 'function')
    throw new Error('Verified local storage and WAV validation are required');
  const bootId = capabilities.bootId, captureStarted = now();
  const report = { bootId, firmwareBuild: capabilities.diagnostics.build, files: [], failure: null,
    firstPlayableFromDownloadMs: null, allPlayableFromDownloadMs: null,
    firstPlayableFromCaptureMs: null, allPlayableFromCaptureMs: null };
  let current = null, downloadStarted = null, lastJob = null;
  try {
    if (options.signal?.aborted) throw new Error('Scan cancelled');
    // Exactly one state-changing START_SCAN. Never automatically retry it.
    const accepted = await link.command('START_SCAN', { bootId });
    if (accepted.bootId !== bootId || typeof accepted.jobId !== 'string' || !/^[a-zA-Z0-9_-]{1,40}$/.test(accepted.jobId)) throw new Error('Invalid scan acceptance');
    report.jobId = accepted.jobId;
    const pollStarted = now();
    let job;
    while (now() - pollStarted < 240000) {
      if (options.signal?.aborted) throw new Error('Scan cancelled');
      job = await link.command('GET_JOB', { bootId, jobId: report.jobId });
      if (job.bootId !== bootId || job.jobId !== report.jobId) throw new Error('Stale job');
      lastJob = job;
      options.onJob?.(job);
      if (job.phase === 'complete') break;
      if (!['selecting','recording','finalizing','resting'].includes(job.phase)) throw new Error(`Scan terminal: ${job.phase}`);
      await sleep(500);
    }
    const manifest = verifyScanManifest(job, bootId, report.jobId);
    report.manifest = manifest;
    downloadStarted = now();
    for (const file of manifest) {
      current = { sensorId: file.sensorId, fileId: file.fileId, nextOffset: 0,
        terminalBulkStatus: null, timing: null };
      const received = await downloadBulk(link, capabilities, file, {
        signal: options.signal, validateWav: options.validateWav,
        onProgress(bytes, total) { current.nextOffset = bytes; options.onProgress?.(file.sensorId, bytes, total); },
        onFailure(failure) { Object.assign(current, failure); }
      });
      current.terminalBulkStatus = received.metrics.firmware;
      current.timing = received.metrics.firmware.timing ?? null;
      const storageStart = now();
      const uri = await options.persistAndReadback(received.bytes, file);
      if (typeof uri !== 'string' || !uri) throw new Error('Missing verified local audio URI');
      const playable = now();
      report.files.push({ ...file, uri, download: received.metrics, storageReadbackMs: playable - storageStart });
      if (report.files.length === 1) {
        report.firstPlayableFromDownloadMs = playable - downloadStarted;
        report.firstPlayableFromCaptureMs = playable - captureStarted;
      }
      // Only verified local tracks are exposed, one at a time, as they finish.
      options.onTrackReady?.(report.files[report.files.length - 1]);
    }
    report.allPlayableFromDownloadMs = now() - downloadStarted;
    report.allPlayableFromCaptureMs = now() - captureStarted;
    report.state = 'complete';
  } catch (error) {
    report.state = 'failed';
    report.failure = { sensorId: current?.sensorId ?? lastJob?.sensorId ?? null,
      nextOffset: current?.nextOffset ?? 0, terminalBulkStatus: null, timing: null,
      ...current, stage: downloadStarted === null ? 'capture' : 'download', error: String(error) };
    // Do not start another sensor/file automatically after a failure.
    if (report.jobId && downloadStarted === null) {
      try { await link.command('STOP_JOB', { bootId, jobId: report.jobId }); } catch { /* preserve original failure */ }
    }
  }
  await options.exportReport?.(report);
  return report;
}

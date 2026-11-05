#line 1 "C:\\Users\\Jerome Vargas\\Downloads\\Ausculta_Recorder_with_SD_Card_Module_All_8_Mics_WORKING__1_\\BLE_BULK_CONTRACT.md"
# Optional finalized-WAV bulk download, version 1

Implemented in firmware; physical iPhone benchmark is pending. Capture commands,
SD files, 16 kHz mono PCM16 WAV format, manifest size/CRC32, and READ_FILE are
unchanged. Old clients may ignore this entire extension. Never enable an
eight-file bulk batch until the one-file physical benchmark below passes.

## Discovery and control

GET_CAPABILITIES adds `bulkDownload`:

```json
{"version":1,"dataUuid":"607d9703-6764-4cc9-b134-70e3c04c8500","ackUuid":"607d9704-6764-4cc9-b134-70e3c04c8500","headerBytes":20,"maxWindowPackets":16,"ackTimeoutMs":10000}
```

Both characteristics belong to the existing 607d9700 service. Data is Notify
with CCCD; ACK is Write With Response. Existing v2 framed JSON control and
request IDs remain on 9701/9702. No binary bytes go on the old JSON lanes.
The client must confirm the data subscription before opening a transfer.

Commands use the existing id/cmd/bootId envelope and ACK/NACK format:

- `BULK_OPEN {bootId,fileId,sensorId}` opens only a current-link completed job's
  immutable file, rejects mismatched sensor, and returns its manifest binding.
- `BULK_STATUS {bootId,transferId}` returns current progress and benchmark metrics.
- `BULK_CANCEL {bootId,transferId}` closes the transfer without changing recordings.

OPEN and STATUS results contain bootId, transferId (uint32, nonzero, unique this
boot), jobId, captureId, fileId, sensorId, byteLength, sampleCount, crc32, state,
error, mtu, valueBytes, payloadBytes, submittedBytes, acknowledgedBytes,
notifications, memoryRetries, elapsedMs, acknowledgedBytesPerSecond.
CRC is the unchanged eight-lowercase-hex CRC32 of the whole original WAV.
`valueBytes=min(actual negotiated ATT MTU-3,512)` and `payloadBytes=valueBytes-20`.
OPEN sends no data until the first binary ACK grants credits. State is initially
awaiting_ack, then sending/awaiting_ack, finally complete/cancelled/failed.

Only one bulk transfer exists. Capture, legacy SD operations and READ_FILE return
BUSY while it owns SD. Cancel or terminal failure releases ownership, allowing
READ_FILE fallback against the same job without disconnecting or recapturing.
An accepted new job still replaces the current manifest, as in the original v2
contract. Do not start a new capture before downloading the required files.

## Binary data (9703)

One complete packet per notification. Little-endian fields:

| Offset | Size | Value |
|---:|---:|---|
| 0 | 4 | ASCII ABW1 |
| 4 | 4 | transferId |
| 8 | 4 | Absolute byte offset in the original WAV |
| 12 | 1 | sensorId, 1-8 |
| 13 | 1 | Flags: bit 0 = final packet, all others zero |
| 14 | 2 | Raw WAV payload length, positive |
| 16 | 4 | Reserved, zero |
| 20 | length | Original file bytes, no Base64 or PCM conversion |

The actual value length is exactly 20+payload length, never above valueBytes.
The final flag is true exactly when offset+length==manifest.byteLength. For
MTU 185 this is up to 162 raw WAV bytes per notification; for MTU 517, 492.
No microphone samples, WAV header, or bytes are regenerated during download.

## Binary cumulative ACK / credits (9704)

Exactly 16 bytes, submitted with response:

| Offset | Size | Value |
|---:|---:|---|
| 0 | 4 | ASCII ABK1 |
| 4 | 4 | transferId |
| 8 | 4 | nextOffset: count of contiguous original WAV bytes accepted |
| 12 | 2 | Next window's packet credits, 1-16 |
| 14 | 2 | Reserved, zero |

After OPEN, send ACK(nextOffset=0, credits=16). Sender submits at most those
16 notifications then waits. After receiving the entire window (or EOF), ACK
the exact next expected offset to grant the next window. No mid-window grants.
The final ACK uses nextOffset=byteLength and valid credits (e.g.16); no further
data is sent. Query BULK_STATUS until complete after that write is acknowledged.
The GATT write response alone is not semantic proof that an ACK was processed.

Reject wrong transfer/sensor, changed packet identity, invalid flags/length,
out-of-range offsets, gaps, overlaps and duplicates. Never ACK bytes beyond the
contiguous received prefix. A stale handle is ignored by firmware; duplicate
cumulative ACKs neither replenish credits nor extend the timeout. A future or
partial-window ACK fails the transfer. There is no network retransmission or
cross-link resume in version 1: on missing packets, cancel then use READ_FILE.

Flow is bounded by 16 packets (at most 8192 notification bytes) outstanding.
Notifications are locally paced at 4 ms minimum, with up to eight 32 ms deferred
retries only when the stack rejects a submission with ENOMEM. Accepted packets
are never locally retried. No credit means no further file reads or sends.
Each window must progress and be ACKed within 10 seconds. Control JSON responses
take precedence. A transfer error closes the file and releases ownership, and
can be inspected with BULK_STATUS; it does not intentionally disconnect BLE.
Disconnect, v2 subscription retirement, data unsubscribe, or epoch change cancels
the transfer. An MTU reduction below OPEN's value limit fails safely, rather than
truncating bytes. Reopen negotiates a fresh handle/size.

## App integration and acceptance

1. Keep the existing READ_FILE implementation. Select bulk only when capability
   version 1 exists and both GATT characteristics have the advertised properties.
2. Subscribe and confirm notifications, OPEN against a finalized manifest entry,
   and compare every returned identity, byte count and CRC with that manifest.
3. Grant windows, enforce offsets/identity, and assemble the original bytes.
   Verify full-file CRC32, WAV format/rate/sample count, then local write/readback
   before exposing playback or claiming successful phone storage.
4. On bulk failure, preserve evidence and cancel/close it before legacy fallback.
   Do not silently retry capture. Discard all unverified partial buffers.

`app-handoff/bulkDownload.mjs` is a dependency-free reference receiver/orchestrator
with an adapter boundary for the existing app BLE manager. It is not installed in
the Mac app; the teammate must integrate and physically verify it. Its validateWav
callback must use the app's existing strict WAV validator. Adapter notifications
are raw decoded bytes; Base64 wrapping belongs only to react-native-ble-plx.

## One-WAV benchmark on the same iPhone 12

Use a single finalized Sensor 1 file with exactly 640044 bytes / 320000 samples.
Keep app foreground, same vest/phone/range/power setup. Download using READ_FILE
and bulk on the same link/job before any new capture. Use identical file bytes.
Record wall-clock download time from first request/grant to last accepted byte,
CRC/WAV validation time, local persistence time separately, negotiated ATT MTU,
payload/window size, accepted bytes, notifications, retries, disconnects, CRC and
audible playback. Firmware elapsedMs measures first grant processing to final ACK
processing; it is not a substitute for the phone's own timing.

No physical numbers have been measured by this agent. Fill this table from the
actual phone export; do not use host-test runtime as a BLE throughput result.

| Measurement | READ_FILE | Bulk |
|---|---|---|
| Device / app build / firmware | pending | pending |
| FileId / sensor / byte count / CRC | pending | pending |
| ATT MTU / raw bytes per packet | pending | pending |
| Download seconds / useful bytes per second | pending | pending |
| CRC+WAV validation / local write+readback seconds | pending | pending |
| Notifications / ENOMEM retries / disconnects | pending | pending |
| Byte-for-byte match / CRC / audible playback | pending | pending |

Pass requires both files identical to the SD WAV, unchanged CRC/sample count,
verified local file and playback, with no missing ranges. Only then consider
enabling an eight-file bulk run. Current firmware supports individual file
transfers, not an automatic eight-file bulk batch.

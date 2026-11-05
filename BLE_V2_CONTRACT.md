# AUSCULTA+ BLE v2 control and finalized WAV contract

Status: app implementation and deterministic tests. Firmware implementation and physical interoperability require separate verification. This document defines the app's v2 wire format. It supersedes earlier draft v2 examples for this implementation. The unused AUS2 draft is unrelated.

## GATT and link setup

| UUID | Properties | Direction |
| --- | --- | --- |
| 607d9700-6764-4cc9-b134-70e3c04c8500 | Primary service | v2 |
| 607d9701-6764-4cc9-b134-70e3c04c8500 | Write with response | Phone → vest |
| 607d9702-6764-4cc9-b134-70e3c04c8500 | Notify, CCCD 0x2902 | Vest → phone |

Retain A001–A006 and the existing v1 handshake and payloads. The current app connects through v1 first (including its existing minimum ATT MTU 179), then explicitly discovers the v2 characteristics and subscribes to the response lane when the user enters physical capture. The v2 frames themselves fit ATT MTU 23 and never go on the v1 characteristics. The firmware should process v2 commands only on the active subscribed link and leave v1 capture idle during v2 acquisition. A GATT write response alone never means the command executed.

## Framing

One UTF-8 JSON object per logical request or response. No newline or terminator. Each ATT value is 5–20 bytes. The 4-byte little-endian header contains uint16 total UTF-8 byte length at offset 0 and uint16 zero-based byte offset at offset 2. Data starts at offset 4. Total is 1–4096 bytes. Nonfinal fragments carry exactly 16 bytes. Final carries 1–16. Offsets start at zero and strictly advance by payload length. A message cannot interleave with another on the same lane. Drop malformed assemblies and retire the connection. Reset assemblies on disconnect or subscription teardown. Base64 transport wrapping by react-native-ble-plx is internal to the phone. Firmware sends the raw frame bytes. READ_FILE has an additional base64 field inside its JSON, specified below.

Only one request is in flight. Request ID is a positive integer assigned by the phone and unique within its link. Response echoes that ID. The phone requires both the completed write sequence and the matching semantic reply within four seconds per request. A reply can arrive before the last native write resolves. Firmware must emit only one response per request and pace all notifications in order. No unsolicited messages in this revision. Progress and finalization are events represented by GET_JOB snapshots. A stale reply while a request is outstanding fails the exchange. No automatic retry of state-changing commands.


{"id":1,"cmd":"GET_CAPABILITIES"}
{"id":1,"ok":true,"result":{"version":2,"bootId":"b42","ready":true,"sampleRate":16000,"format":"wav-pcm16-mono","sensorIds":[1,2,3,4,5,6,7,8],"modes":["single","scan"]}}
{"id":2,"cmd":"SELECT_SENSOR","bootId":"b42","sensorId":3}
{"id":2,"ok":true,"result":{"bootId":"b42","sensorId":3}}
{"id":3,"ok":false,"error":"BUSY"}


NACK error codes are uppercase strings: BAD_REQUEST, UNSUPPORTED, STALE_BOOT, BUSY, NOT_READY, SENSOR_MISMATCH, NOT_FOUND, RANGE, CAPTURE_FAILED. The app currently shows a generic rejected-command error. Every successful response after capability discovery contains the same bootId inside result. All IDs emitted by the vest are case-sensitive ASCII [a-zA-Z0-9_-], 1–40 characters. IDs must be unique in their relevant namespace within a boot. bootId changes on every reboot. No patient information goes to the vest. Unknown fields must not change the defined semantics.

## Commands
| cmd | Fields besides id/cmd | Successful result besides bootId |
| --- | --- | --- |
| GET_CAPABILITIES | None | See example. ready means the firmware can start a new capture, including I2S, mux, SD and buffer readiness, with no conflicting owner. Advertise scan only after firmware implements it. |
| SELECT_SENSOR | bootId, sensorId (integer 1–8) | sensorId confirmed after mux selection and settling. Reject while a capture or scan owns the recorder. |
| START_CAPTURE | bootId, sensorId | jobId. Require the same confirmed sensor, readiness, and no other capture owner. Start a single capture of up to 320000 samples (20 seconds at 16 kHz). |
| START_SCAN | bootId | jobId. Fixed order 1–8, up to 320000 samples per sensor, 5000 ms rests between sensors. No trailing rest. Firmware owns sequencing. |
| GET_JOB | bootId, jobId | jobId, phase. In selecting/recording/finalizing/resting include sensorId. In complete include files. Terminal partial/failed/cancelled may include reason. |
| STOP_JOB | bootId, jobId | jobId, phase=stop_requested. Stop current capture or rest and prevent further sensors. Acknowledge the request separately from terminal state. |
| READ_FILE | bootId, fileId, offset, length | fileId, offset, data (standard padded base64 of exactly length raw WAV bytes). Only for a finalized immutable file from the current link's completed job. |

START returns a job acceptance, not evidence that acquisition or storage succeeded. Bind job, files and mux selection to the active BLE link and share the existing Wi-Fi/BLE capture ownership guard. Schedule I2S, mux, SD finalization and file reads outside BLE callbacks. Jobs accepted on a retired link must stop without starting more sensors. Reject access to unrelated or prior-link files. Existing Wi-Fi/SD recordings are preserved. Completion means all expected WAVs closed successfully, and a failed or interrupted job must report failed, partial or cancelled. The app does not publish partial v2 jobs in this release.

### Progress and finalization examples


{"id":4,"cmd":"GET_JOB","bootId":"b42","jobId":"j1"}
{"id":4,"ok":true,"result":{"bootId":"b42","jobId":"j1","phase":"recording","sensorId":3}}
{"id":5,"ok":true,"result":{"bootId":"b42","jobId":"j1","phase":"complete","files":[{"jobId":"j1","captureId":"c3","sensorId":3,"fileId":"f3","sampleCount":320000,"byteLength":640044,"crc32":"01234567"}]}}


The CRC in the example is illustrative. A single job returns exactly one file with the selected sensor. A scan returns exactly eight files in sensor order, with distinct captureId and fileId for every track. The manifest binds bootId (outer result), jobId, captureId, sensorId and fileId. Each result is derived from actual audio acquired at that mux position. Expose complete job metadata until the link retires. Phone polls approximately every 500 ms and bounds job polling to 35 s for single and 240 s for scan (plus an in-flight request). The app treats an unknown phase or sensor identity as failure.

## File transfer and integrity

WAV is RIFF/WAVE PCM format 1, mono signed PCM16 little-endian, 16000 Hz. File size 46–640044 bytes inclusive, 1–320000 samples, with valid RIFF length, fmt/data chunks, PCM alignment and sample count. Use a canonical 44-byte header for a full-length file. Manifest crc32 is exactly eight lowercase hex digits, CRC-32/ISO-HDLC over all finalized file bytes (reflected polynomial 0xEDB88320, initial/final XOR 0xFFFFFFFF; test vector ASCII 123456789 → cbf43926). CRC detects corruption and does not authenticate a peer or file.
Phone uses sequential READ_FILE requests at offsets 0, 768, 1536, … with length min(768, remaining bytes). Firmware returns the same fileId and offset with the exact requested byte count encoded in base64. No padding bytes beyond the range. The app verifies the response identity and length, checks full-file CRC, and validates sample count and rate against the WAV. It writes the original bytes to the owning patient's local storage and reads them back before publishing. No hosted sync for this path. Cancelled, corrupt, mismatched or failed batches publish no completed session. No cross-link resume or auto-retry in this revision. After disconnect, start with fresh capability discovery and an explicit new capture. The app never guesses the fate of an unconfirmed START or tries it again automatically.

## Acceptance order

1. Confirm v1 connection remains compatible. Verify v2 capabilities and one explicit SELECT_SENSOR ACK.
2. Start a single sensor, observe progress, finalize, transfer, verify and play its WAV. Check wrong-sensor, corrupted file and stop cases.
3. Enable scan capability only on firmware that performs the verified sequential scan. Verify all eight physical positions, rests, distinct WAVs and correct association.
4. Exercise disconnect during capture/rest/transfer, patient retirement, SD failure, and contention with the existing Wi-Fi recorder. v2 is a bench contract. Peer authentication and real clinical acceptance require separate validation.

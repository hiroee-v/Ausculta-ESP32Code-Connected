# Eight-sensor acceptance on v2-bulk-timing-3

Do not reflash or tune the accepted firmware baseline for this run. A source-only
snapshot and SHA256 manifest are retained under baselines/v2-bulk-timing-3.
It pins the local source reported to pass one physical file, not a readback/hash
of the actual device. Existing Arduino board settings remain authoritative.

Source inspection: START_SCAN creates one job; its task loops logical sensors
1-8, with a 5-second rest only between sensors. Each successful finalization
records a distinct captureId/fileId, its actual sampleCount/byteLength and CRC32.
The complete manifest is published only after all eight finalizations succeed.
STOP/disconnect/failure cannot publish a successful full scan. This inspection
is not a substitute for a physical eight-sensor acceptance trace.

Use app-handoff/eightSensorRun.mjs as a reference with the existing BLE adapter.
It issues START_SCAN exactly once, polls the same job, validates the eight-entry
manifest, then opens/downloads/closes each file sequentially on the existing
bulk protocol. No parallel transfers, new capture, window/payload/pacing change,
or protocol extension. The app must integrate this reference; it is not running
on the Mac/iPhone from this workspace.

Pass the existing strict WAV validator and verified patient/session-owned local
write/readback function. Publish each playable track only after those checks.
Report both download-start and capture-start timings for first/all available
tracks. “Playable” here means a verified local URI is available; actual audible
playback still requires a human/device check. Preserve explicit user cancellation.

On failure the report includes sensorId, fileId, acceptedBytes/nextOffset,
terminalBulkStatus and timing. If the BLE link is lost, status can be null; do
not fabricate terminal counters. Stop the sequence and export the failure before
fallback traffic replaces evidence. Existing READ_FILE fallback remains available
after a confirmed bulk closure on the same link; do not recapture automatically.

Required physical evidence:

1. Capabilities build/bootId and exactly one START_SCAN ACK/jobId.
2. Timestamped phases for sensors 1-8 and eight distinct finalized identities.
3. Per-file byte counts/CRCs, offsets, terminal bulk status and timing counters.
4. Original-byte CRC/WAV/sample-count checks, local readback and audible playback
   for every track; no assumptions from SD capture success alone.
5. Time to first verified playable track and all eight, with clear time origins.

All physical eight-file results are pending. Host tests verify the reference
runner with eight simulated WAVs, one START_SCAN, strictly sequential downloads,
identity/manifest rejection, verified readback hooks, and a failure on Sensor 3
that prevents Sensors 4-8 from starting. No host runtime is a phone speed result.

Live dialogs must follow VEST_LIVE_STATUS.md. In particular battery measurement
is unavailable and legacy GET_STATUS.recording does not reflect v2 acquisition.

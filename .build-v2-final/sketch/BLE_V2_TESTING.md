#line 1 "C:\\Users\\Jerome Vargas\\Downloads\\Ausculta_Recorder_with_SD_Card_Module_All_8_Mics_WORKING__1_\\BLE_V2_TESTING.md"
# AUSCULTA BLE v2 bench test

The workspace implements the supplied BLE_V2_CONTRACT.md. A001-A006 remains for
the original connection. The second service is
607d9700-6764-4cc9-b134-70e3c04c8500; 9701 is write-with-response and 9702 notify.
Use the updated v2 app, not the old unconditional physical-recording block.

## Before upload

- Verify for the actual ESP32-S3 board and PSRAM configuration. The recorder
  buffer needs 640,000 bytes; v2 request history additionally needs 128 KiB PSRAM.
  A PSRAM allocation failure disables v2 and is logged.
- SD must initialize. Battery reporting remains unavailable. Readiness reflects
  driver initialization, configured mux GPIOs, SD, memory and recorder ownership;
  it does not prove microphone wiring or signal quality.
- Compilation and wire tests do not verify hardware or app interoperability.
  The coding agent has not flashed the board.

## First physical test: one microphone

1. Upload, open Serial Monitor at 115200, and reset. Check SD and I2S initialization
   and `[V2] Service ready; boot b...`.
2. Disconnect any scanner app. Connect in AUSCULTA. The legacy connection still
   requires negotiated ATT MTU >= 179.
3. Enter physical capture. Require ready:true from GET_CAPABILITIES and the
   requested sensor ID in SELECT_SENSOR's ACK.
4. Start a single capture. GET_JOB should progress through selecting, recording,
   finalizing and complete. Recording runs for up to 20 seconds.
5. Look for `[V2] Finalized f... sensor ...`. Manifest publication follows WAV
   close, reopen, exact-size check and CRC verification against written bytes.
6. Wait for READ_FILE transfer, phone integrity verification and local storage.
   Play the WAV. Check physical microphone identity, then repeat for sensors 1-8.

v2 transfers finalized WAVs, not live v1 PCM. Zero v1 stream chunks during v2
capture is expected. GET_JOB polls carry progress; there are no unsolicited events.
The app must display v2 capture/download state separately from v1 streaming stats.

## Scan and failure checks

- START_SCAN runs 1-8 with up to 320,000 samples per sensor and 5-second rests,
  without a trailing rest. Require eight distinct sensor/capture/file identities,
  correct physical mapping and eight verified WAVs before accepting the session.
- STOP during recording/finalization/rest must prevent complete and the next
  sensor. Partial audio may remain on SD. Partial/cancelled jobs are not available
  through READ_FILE or publishable as completed app sessions.
- Disconnect/unsubscribe during acquisition: stop without starting another sensor.
  Reconnect: old job/file IDs must return NOT_FOUND. Reboot changes bootId;
  requests with the old bootId must return STALE_BOOT.
- Exercise missing/full SD and read/write failures. They cannot produce complete.
  An SD finalization/read failure disables readiness until reset/remount.
- During a v2 job, web mic/start/scan/stop and SD listing/file routes return 409.
  BLE v2 START during Wi-Fi recording returns BUSY. V1 commands cannot take over
  v2 acquisition. HTTP audio playback is blocked while subscribed to v2 responses;
  disconnect the app before using HTTP playback.

## Transfer limits and preserved files

READ_FILE accepts 1-768 bytes in range, only from the current link's completed job.
Responses use padded Base64 inside framed JSON. Values have the 4-byte little-
endian header and at most 16 data bytes. Notifications are paced at least 4 ms
apart. Submission success is not proof of receipt; the phone verifies coverage,
CRC and WAV format. Notification/protocol faults retire the link without retry.

This small framing can take several minutes per 640 KB WAV, substantially longer
for eight files. Measure throughput on the iPhone. Capture completion and download
completion are separate. Faster transport needs an agreed app/firmware revision.

One job manifest is retained per link; download before starting another job.
Disconnect retires file access but does not delete recordings. V2 files use unique
BLEV2_<boot>_<job>_<file>.wav names. Existing web/SD files are preserved.
Request IDs cannot repeat; reconnect after 16,384 requests (sufficient for one
full eight-file job plus polling). No cross-link resume is defined by the contract.

## Software checks

tests/ble_v2_wire_test.cpp tests the exact shared firmware wire primitives:
round trips from 1 to 4096 bytes, fragment boundaries, malformed/out-of-order/
duplicate fragments, changed totals, range limits, and the CRC32 123456789 known
vector and incremental calculation. Physical acceptance remains pending.

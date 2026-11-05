#line 1 "C:\\Users\\Jerome Vargas\\Downloads\\Ausculta_Recorder_with_SD_Card_Module_All_8_Mics_WORKING__1_\\app-handoff\\README.md"
# App teammate handoff

Read ../BLE_BULK_CONTRACT.md first. Keep all existing capture commands and the
working READ_FILE receiver. Integrate bulkDownload.mjs behind capability version
1 and a development-only single-file benchmark flag. Do not enable an automatic
eight-file bulk batch yet.

Adapt the existing react-native-ble-plx manager to the three methods documented
at the top of bulkDownload.mjs. Decode notification Base64 to Uint8Array and
encode ACK Uint8Array to Base64 only at the native BLE adapter boundary. Preserve
existing connection epoch, patient/session retirement, and cancellation checks.
Confirm native subscription readiness; serialize ACK writes with response.

Pass the app's strict WAV validator as validateWav and an AbortSignal for explicit
download cancellation. Persist and read back returned bytes using the existing
patient/session-owned file path before showing playback/Save/Export. Do not
publish completed audio solely because firmware BULK_STATUS says complete.

Export raw control replies and packet metadata (handle, sensor, offset, length),
not patient data or audio samples. Capture wall-clock transfer and verification/
storage durations separately. Use a 640044-byte WAV on the same physical iPhone
12, compare bytes/CRC and playback with READ_FILE, and fill the benchmark table.
Return the actual measured JSON/results to the firmware teammate. Host unit tests
are not a physical BLE speed test.

Current firmware marker: diagnostics.build = v2-bulk-timing-2. Read
../BULK_TIMING_UPDATE.md for the callback correction and additive timing counters.
BULK failures intentionally release
the file without disconnecting, allowing the existing READ_FILE path after
BULK_CANCEL (or a confirmed terminal state). Do not silently start another capture.

Local reference checks: `node tests/bulk_receiver.test.mjs` from firmware workspace.
The firmware capture functions, PCM format and SD finalization were not changed.

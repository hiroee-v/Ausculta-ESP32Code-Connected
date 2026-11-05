#line 1 "C:\\Users\\Jerome Vargas\\Downloads\\Ausculta_Recorder_with_SD_Card_Module_All_8_Mics_WORKING__1_\\BLE_TESTING.md"
# AUSCULTA app connection check

This page describes the legacy v1 path. For the new single/eight-sensor v2 app,
use [BLE_V2_TESTING.md](BLE_V2_TESTING.md). The limitations below refer to v1 only.

The sketch implements the supplied A001 service, A002 write-with-response commands,
and A003-A006 notification characteristics. Commands are raw UTF-8. PING sends a
valid status notification, not PONG. Battery and sensor notification lanes remain
silent: no battery measurement exists and audio is sent on A006.

Compile for your actual ESP32-S3 board and PSRAM configuration before uploading.
The existing 20-second audio buffer requires 640,000 bytes. No upload was performed
by this change.

1. Upload the verified sketch, open Serial Monitor at 115200 baud, and reset.
2. Disconnect any BLE scanner app, then rescan from the AUSCULTA app.
3. Connect. Expect PING, GET_STATUS, GET_BATTERY logs and negotiated MTU >= 179.
4. Start a recording. Confirm recording status arrives within the app's 1-second
   timeout and the app receives real PCM with I2S provenance at 16000 Hz.
5. Stop early. Confirm final PCM has flag 0x0028 and status recording:false follows.
6. Repeat with the automatic 20-second end, then disconnect/reconnect and repeat.
7. Check packet gaps in the app, actual recording duration, playback, and SD file.
   Notification submission is not proof of receipt. Measure throughput on hardware.

The phone assembles its own WAV from received PCM. It cannot retrieve SD files
through this BLE protocol. Existing Wi-Fi recording/download routes remain.
Avoid HTTP playback/download while testing streaming because file serving blocks
the main loop that submits BLE frames. Streaming is buffered by the capture buffer.

The mobile protocol has no microphone-selection or eight-microphone scan command.
START_RECORDING uses the currently selected microphone (sensor 1 after reset).
The web interface can select a microphone while idle. Eight-zone app control needs
an agreed app/firmware protocol extension; the PCM header has no microphone ID.

sensorReady reports successful I2S driver setup plus available capture memory.
It does not detect missing microphones or validate signal quality. A capture with
no I2S samples cannot emit a valid zero-sample stop frame; inspect serial/I2S wiring
if the app reports interrupted audio. Battery remains unavailable in the app.

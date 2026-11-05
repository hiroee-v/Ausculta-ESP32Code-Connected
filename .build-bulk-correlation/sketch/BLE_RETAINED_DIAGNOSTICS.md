#line 1 "C:\\Users\\Jerome Vargas\\Downloads\\Ausculta_Recorder_with_SD_Card_Module_All_8_Mics_WORKING__1_\\BLE_RETAINED_DIAGNOSTICS.md"
# Retrieve a transfer failure without Serial Monitor

Firmware build marker: `diagnostics.build = "v2-backpressure-1"` in the existing
GET_CAPABILITIES result. All UUIDs, framing, commands, transfer sizes and timeouts
remain unchanged. Notifications are paced at 16 ms minimum. A locally rejected
NimBLE BLE_HS_ENOMEM submission waits 32 ms before resubmitting the identical
fragment, without advancing the payload offset. Up to eight deferred retries per
fragment are allowed, still bounded by the original 3.5-second response deadline.
Other failures keep the diagnostic fault path. No command or successfully queued
fragment is retried. `notificationRetriesSinceBoot` counts deferred send attempts.

1. Flash this workspace, then use the existing working battery/SD power setup.
2. Run one single-sensor recording through file download with phone tracing armed.
3. On failure, preserve the timeline. Do not reset or power-cycle the vest.
4. Reconnect and request GET_CAPABILITIES only, without starting a new recording.
5. Export the complete decoded result, preserving `bootId` and `diagnostics` even
   if the app's typed capability object strips unknown fields.

`diagnostics.lastDisconnect` is null until a disconnect has been observed since
boot. Otherwise it contains a snapshot of the last disconnected link:

- `firmwareFault` and `fault`: whether a local v2 fault was recorded before the
  disconnect callback; a true value is evidence of a firmware fault path, not
  proof that this was the only reason the radio link dropped.
- `disconnectReason`: raw ESP32 stack reason, or -1 if unavailable.
- `requestId`, `command`, `fileId`, `fileOffset`: last parsed command context.
- `responseOffset`, `responseLength`: JSON response payload bytes submitted to
  the stack, excluding the four-byte per-fragment headers. These are not WAV
  byte counts and are not proof of phone receipt. A concurrent disconnect can
  snapshot the previous submission boundary.
- `notifyStatus`, `notifyCode`: most recent v2 notification callback status and
  raw stack code for that command; status -1 means no callback observed yet.
  Status 1 is SUCCESS_NOTIFY in the tested Arduino ESP32 core 3.3.12. Status 4 is
  ERROR_GATT. Code interpretation must use that installed stack's definitions.
- `freeHeap`, `minHeap`: bytes sampled at disconnect (minimum is since boot).
- `loopStackFree`: task stack high-water mark sampled at command dispatch, in
  the installed ESP-IDF FreeRTOS implementation's units.
- `atMs`: milliseconds since boot when the disconnect snapshot was captured.

Snapshots are held only in RAM. Reboot/power loss clears them and changes bootId.
The next disconnect replaces the previous snapshot. Export the first reconnect
reply before further tests. No microphone samples, credentials or patient IDs are
included. Existing protocol integrity checks and recorder ownership remain.

The compatibility adjustment for restored CCCD subscriptions remains in place.
No automatic recording retry, cross-link file resume or fabricated completion
has been added.

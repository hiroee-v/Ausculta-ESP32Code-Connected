# v2-bulk-timing-2: failed baseline investigation

Physical baseline supplied by the app teammate: 236 accepted 492-byte payloads,
116112 contiguous phone bytes, 110208 cumulatively ACKed bytes, 48 local memory
retries, then generic NOTIFICATION_SUBMIT. No bulk disconnect; later fallback
was explicitly cancelled. Full-file speed, CRC and playback did not pass.

Source inspection of Arduino ESP32 3.3.12 shows BLECharacteristic::notify calls
onStatus immediately, while BLEServer also calls the SAME onStatus for subsequent
BLE_GAP_EVENT_NOTIFY_TX. The former shared variable could confuse an asynchronous
TX event with the current call's return. This is a verified source-level race,
not proof of the native error that ended the old baseline (not retained there).

For NimBLE the new sender directly calls ble_gatts_notify_custom and checks its
return for submission acceptance. Its mbuf is consumed by that API even on error.
onStatus now counts only native TX events. At most one accepted local notification
is pending TX completion; that is distinct from the unchanged 16-packet receiver
credit window. Native completion does not prove receipt by the phone.

Pacing stays 4 ms. ENOMEM backoff stays 32 ms, at most 8 retries, and the window
deadline stays 10 s. No accepted packet is retried. Native TX errors fail the
transfer with NATIVE_TX_ERROR. Exact rejected submission code is retained. No
recording format, capture code, CRC, binary header, UUID or ACK contract changed.

BULK_STATUS/OPEN add a timing object. All duration counters are microseconds:
- sdReadUs: accumulated esp_timer_get_time deltas around each seek/read pair.
- notifySubmitUs: around mbuf allocation plus native notification API call.
- pacingWaitUs: loop intervals classified as minimum-gap/backoff deferrals.
- ackCreditWaitUs: intervals with no receiver credits or all file bytes submitted,
  awaiting a valid ACK; includes initial-grant waiting before elapsedMs starts.
- txWaitUs: intervals awaiting a native TX event before the next send.

Wait counters use successive processBulkBluetooth observations: time from a
defer decision until its next loop visit, including intervening loop scheduling,
is attributed to that category. They do not measure radio airtime or pure sleep.
Pending wait time is flushed on the next loop visit/close; a live STATUS can lag
by one visit. SD and submission durations are explicit call spans. Control reply
priority time is not classified as pacing or TX wait.

Additional counters: submitAttempts, rejectedNoMemory, rejectedOther,
lastSubmitCode, nativeTxEventsAvailable, nativeTxEvents, nativeTxErrors,
lastNativeTxError, outstandingSubmissions. Counted rejection categories identify
ENOMEM versus other codes; the last raw other code is retained, not a full error
histogram. Native event counters are guarded against concurrent BLE callbacks.
Rejected submissions never increase outstandingSubmissions. On non-NimBLE builds
nativeTxEventsAvailable=false and no completion-based throttle is claimed.

App teammate: retain the new timing fields in raw STATUS exports. Preserve ACK
timings and bulk metadata on FAILURE before fallback starts. Run the same ONE
640044-byte Sensor 1 file again. Report the 515 (phone snapshot) versus 517
(firmware) MTU observations separately; both permit the observed 512-byte value.
Do not guess a radio negotiation failure from that difference. Do not run eight
files or claim improved speed until complete bytes/CRC, readback and audible
playback pass. No physical benchmark has been run by the firmware agent.

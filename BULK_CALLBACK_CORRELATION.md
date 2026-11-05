# v2-bulk-timing-3: correlate callback and native submission

The physical report showed eight accepted notifications, then submission attempt
9 returning ENOMEM (6) while a callback also carried 6. The old tracker counted
that callback as a failed completion even though the native API rejected the
packet. This incorrectly triggered NATIVE_TX_ERROR on the next loop visit.

The sender now marks the native call in progress before calling notify_custom.
Callbacks arriving during that call are deferred under the existing critical-
section lock until its return value is known:

- Return 0: count one accepted submission. A deferred event completes that accepted
  submission, and only its nonzero event status can become a fatal TX error.
- Return nonzero: do not increment accepted, completed, or fatal TX error counts.
  A deferred callback is recorded as belonging to that rejected attempt. ENOMEM
  follows the EXISTING eight-retry / 32 ms backoff path, retaining pendingBytes,
  the exact packet buffer and the WAV offset. Credits are not consumed.
- Events observed with no accepted notification pending and no call in progress
  are counted as unowned callbacks and do not become accepted TX failures.

At most one accepted submission can be outstanding. Callback processing and
return classification share a lock. The API itself is not called inside the
critical section. An accepted packet is never retransmitted by this mechanism.
This resolves inline-return races; a callback lacking any per-packet token cannot
be assigned arbitrarily to a future packet. Unexpected unowned callbacks remain
visible in diagnostics for investigation.

BULK_STATUS adds timing.rejectedSubmissionCallbacks, timing.unownedTxCallbacks,
and timing.lastCallbackAttempt (1-based local API attempt). Existing lastSubmitCode,
rejection counters, nativeTxErrors, nativeTxEvents and outstandingSubmissions
remain available. Native event counts are not proof of radio delivery or phone
receipt; compare them with cumulative ACKs and the phone's contiguous byte count.

No UUID, packet/ACK layout, WAV bytes, CRC, sensor/file binding, capture behavior,
4 ms pacing, 16-packet receiver window, eight-retry limit or timeout changed.

Regression checks replay eight successes, a rejected ninth call with an inline
ENOMEM callback, a successful retry, an asynchronous accepted TX error, and an
unowned callback after rejection. Legacy framing/CRC and full-file bulk receiver
checks are retained. Physical single-file acceptance remains pending.

App teammate: confirm diagnostics.build=v2-bulk-timing-3, preserve ALL timing
fields on failure, and rerun only the same 640044-byte Sensor 1 test. Expected
rejection behavior is unchanged submittedBytes/offset during retry, no false
NATIVE_TX_ERROR, and rejectedSubmissionCallbacks increasing for an inline
callback attached to a rejected native call. Do not run eight files until full
byte count, CRC/WAV validation, phone readback and audible playback pass.

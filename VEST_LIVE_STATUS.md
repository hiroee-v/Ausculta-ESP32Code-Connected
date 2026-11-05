# Live vest details on the preserved v2-bulk-timing-3 baseline

This documents existing responses; it does not add or change firmware fields.
The app must display unavailable measurements explicitly rather than inventing
numbers. The reported physical single-file pass is user-provided; an eight-file
physical run is still pending.

## What is measured or known

- Audio samples are acquired from the configured I2S input after GPIO mux
  selection. Filtered/gained PCM is saved as 16000 Hz, mono PCM16 WAV.
- sampleCount is acquired sample count; byteLength and CRC32 are verified from
  the finalized SD file. Duration is sampleCount/16000, not the UI timer.
- Sensor identity is the firmware's logical mux position 1-8. There is no
  electrical presence/health test for each microphone. Confirm physical mapping
  with the individual-mic sound test before treating labels as verified wiring.
- Readiness is software/driver/storage/memory/ownership state, not a measured
  clinical quality indicator or proof that all eight microphones work.
- No battery ADC input/divider configuration, calibrated voltage conversion or
  fuel-gauge reader exists in this firmware. Battery voltage, percentage and
  charging state are unavailable. Battery powering the circuit is not sensing.

## Exact legacy status and battery lanes

Write raw UTF-8 GET_STATUS on A002 (not a framed JSON v2 command). A003 notifies:

```json
{"type":"status","connected":true,"recording":false,"sensorReady":true}
```

| Field | Type | Actual meaning |
|---|---|---|
| type | string | status |
| connected | boolean | true while this BLE link can receive the response |
| recording | boolean | legacy recording/scanning or legacy BLE buffer drain |
| sensorReady | boolean | I2S driver initialization succeeded AND audio buffer allocated |

Important: recording excludes the v2 job owner. It can be false while START_SCAN
is capturing. sensorReady excludes SD readiness and current ownership. Do not use
these legacy flags alone to drive v2 recording dialogs. No activeSensor field is
present. A disconnected peer cannot deliver a connected:false notification;
use the app's native connection state for link loss.

Write GET_BATTERY on A002. A005 remains notifiable, but this hardware build sends
NO battery notification because no measurement exists. Its legacy parser accepts
only {"type":"battery","level":number}; null/unsupported payloads must not be
invented on that lane. On this known baseline, show “Battery unavailable — not
instrumented”, with no percentage, voltage or charging claim. Silence alone on an
unknown future build is not proof that its battery hardware is unsupported.

Suggested APP dialog state (not an existing BLE response):

```json
{"battery":{"available":false,"reason":"not_instrumented","voltageV":null,"percent":null,"charging":null}}
```

Real measured battery reporting needs an identified measurement circuit/gauge
and a separately tested firmware/app extension. Do not change the accepted scan
baseline merely to supply fabricated telemetry.

## Authoritative v2 state

GET_CAPABILITIES on the existing framed JSON control lane returns result:

- bootId: string; firmware boot identity.
- version: 2.
- ready: boolean; idle recorder AND initialized I2S AND initialized SD AND allocated
  audio buffer, including no active bulk storage owner.
- sampleRate: 16000; configured capture rate, not an independent clock measurement.
- format: wav-pcm16-mono.
- sensorIds: [1,2,3,4,5,6,7,8]; supported logical positions.
- modes: [single,scan].
- diagnostics.build: v2-bulk-timing-3.
- bulkDownload: existing version-1 transport capability.

GET_JOB {bootId,jobId} result while active:

```json
{"bootId":"b_example","jobId":"j1","phase":"recording","sensorId":3}
```

phase is selecting, recording, finalizing, resting, complete, partial, cancelled
or failed. In the four nonterminal phases, sensorId is present. During selecting,
it is the target being selected; during recording/finalizing it is the capture's
logical sensor; during resting it is the sensor just recorded. There is no
separate activeSensor or captureState field. Derive the UI state from phase,
not from legacy recording. SELECT_SENSOR ACK confirms sensorId after selection
and settling when idle. In terminal GET_JOB replies sensorId is omitted; clear
the active-recording indicator and use the manifest's per-track identities.

complete supplies files[] with jobId, captureId, sensorId, fileId, sampleCount,
byteLength and crc32. A CRC can legitimately equal another file's CRC; identity
uniqueness applies to sensor/capture/file IDs, not CRC values. Transfer state is
separate: BULK_STATUS.state is awaiting_ack/sending/complete/failed/cancelled.
Capture complete does not mean audio has reached the phone.

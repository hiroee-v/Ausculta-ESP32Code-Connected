#pragma once
// Included after the sketch's recorder functions. ESP32 core supplies cJSON and mbedTLS.
#include "BleV2Wire.h"
#include <cJSON.h>
#include <mbedtls/base64.h>
#include <esp_system.h>
#if defined(CONFIG_NIMBLE_ENABLED)
#include <host/ble_hs.h>
#endif

#define V2_SERVICE "607d9700-6764-4cc9-b134-70e3c04c8500"
#define V2_COMMAND "607d9701-6764-4cc9-b134-70e3c04c8500"
#define V2_RESPONSE "607d9702-6764-4cc9-b134-70e3c04c8500"
void initBulkGatt(BLEService* service);

enum class V2Phase { None, Selecting, Recording, Finalizing, Resting, Complete, Cancelled, Partial, Failed };
struct V2File {
  char captureId[24], fileId[24], path[96];
  uint8_t sensor;
  uint32_t samples, bytes, crc;
};
struct V2Job {
  uint32_t epoch;
  char id[24];
  bool scan;
  uint8_t sensor, count;
  V2Phase phase;
  V2File files[8];
};
struct V2Inbound { uint32_t epoch; uint8_t size; uint8_t bytes[20]; };
BLECharacteristic* v2Response = nullptr;
BLE2902* v2Cccd = nullptr;
QueueHandle_t v2Inbound = nullptr;
SemaphoreHandle_t v2JobMutex = nullptr;
volatile uint32_t v2Epoch = 1;
volatile bool v2Subscribed = false, v2Stop = false, v2Fault = false;
volatile bool v2NotifyAccepted = false;
volatile int v2LastNotifyStatus = -1;
volatile uint32_t v2LastNotifyCode = 0;
uint32_t v2NotifyRetries = 0;
BleV2Wire::SendWindow v2SendWindow;
const char* volatile v2FaultReason = "unspecified";
volatile unsigned v2TraceBudget = 16;
struct V2LinkDiagnostic {
  bool valid = false, disconnected = false, localFault = false;
  double requestId = 0;
  char command[24] = {}, fileId[41] = {}, fault[40] = {};
  uint32_t fileOffset = 0, notifyCode = 0, freeHeap = 0, minHeap = 0, stackFree = 0, atMs = 0;
  uint16_t responseOffset = 0, responseLength = 0;
  int notifyStatus = -1, disconnectReason = -1;
};
portMUX_TYPE v2DiagnosticMux = portMUX_INITIALIZER_UNLOCKED;
V2LinkDiagnostic v2CurrentDiagnostic, v2LastDisconnect;

void beginV2DiagnosticLink() {
  portENTER_CRITICAL(&v2DiagnosticMux);
  v2CurrentDiagnostic = V2LinkDiagnostic{};
  portEXIT_CRITICAL(&v2DiagnosticMux);
}
void recordV2Disconnect(int reason) {
  uint32_t heap = ESP.getFreeHeap(), minimum = ESP.getMinFreeHeap();
  uint32_t at = millis();
  portENTER_CRITICAL(&v2DiagnosticMux);
  if (!v2CurrentDiagnostic.disconnected) {
    v2CurrentDiagnostic.valid = true;
    v2CurrentDiagnostic.disconnected = true;
    v2CurrentDiagnostic.disconnectReason = reason;
    v2CurrentDiagnostic.freeHeap = heap;
    v2CurrentDiagnostic.minHeap = minimum;
    v2CurrentDiagnostic.atMs = at;
    v2LastDisconnect = v2CurrentDiagnostic;
  }
  portEXIT_CRITICAL(&v2DiagnosticMux);
}
void v2Fail(const char* reason) {
  portENTER_CRITICAL(&v2DiagnosticMux);
  if (!v2CurrentDiagnostic.localFault) {
    v2CurrentDiagnostic.localFault = true;
    strlcpy(v2CurrentDiagnostic.fault, reason, sizeof(v2CurrentDiagnostic.fault));
  }
  portEXIT_CRITICAL(&v2DiagnosticMux);
  v2FaultReason = reason;
  v2Fault = true;
}
V2Job v2Job = {};
BleV2Wire::Assembly v2Rx;
String v2Tx;
uint16_t v2TxOffset = 0;
uint32_t v2LocalEpoch = 0, v2LastRxAt = 0, v2LastTxAt = 0, v2TxStarted = 0;
uint32_t v2NextJob = 1, v2NextCapture = 1;
uint8_t v2ConfirmedSensor = 0;
bool v2CapabilitiesKnown = false;
char v2Boot[24];
double* v2RequestIds = nullptr;
size_t v2RequestCount = 0;

void retireV2Link() {
  v2Subscribed = false;
  v2Stop = true;
  ++v2Epoch;
}

class V2SubscriptionCallbacks : public BLEDescriptorCallbacks {
  void onWrite(BLEDescriptor*) override {
    bool enabled = v2Cccd->getNotifications();
    bleLogf("[V2] CCCD notify=%u epoch=%lu\n", enabled, (unsigned long)v2Epoch);
    if (!enabled) retireV2Link();
    else { v2Subscribed = true; v2TraceBudget = 16; }
  }
};
class V2WriteCallbacks : public BLECharacteristicCallbacks {
  void onWrite(BLECharacteristic* characteristic) override {
    auto value = characteristic->getValue();
    // Bonded reconnects can restore the CCCD via BLE2902::restoreValue(),
    // which calls setValue() without invoking our descriptor-write callback.
    // Honor the stack's actual enabled state rather than rejecting that link.
    if (!v2Subscribed && bleDeviceConnected && v2Cccd->getNotifications()) {
      v2Subscribed = true;
      bleLogf("[V2] Using enabled CCCD without a new subscription callback.\n");
    }
    if (v2TraceBudget) {
      --v2TraceBudget;
      const uint8_t* bytes = reinterpret_cast<const uint8_t*>(value.c_str());
      unsigned total = value.length() >= 4 ? bytes[0] | (unsigned(bytes[1]) << 8) : 0;
      unsigned offset = value.length() >= 4 ? bytes[2] | (unsigned(bytes[3]) << 8) : 0;
      bleLogf("[V2] WRITE size=%u total=%u offset=%u subscribed=%u connected=%u epoch=%lu\n",
              unsigned(value.length()), total, offset, unsigned(v2Subscribed),
              unsigned(bleDeviceConnected), (unsigned long)v2Epoch);
    }
    if (!v2Subscribed) { v2Fail("write_before_subscription"); return; }
    if (!bleDeviceConnected) { v2Fail("write_without_link"); return; }
    if (value.length() < 5 || value.length() > 20) { v2Fail("write_size"); return; }
    V2Inbound frame = {};
    frame.epoch = v2Epoch;
    frame.size = value.length();
    memcpy(frame.bytes, value.c_str(), frame.size);
    if (xQueueSend(v2Inbound, &frame, 0) != pdTRUE) v2Fail("receive_queue_full");
  }
};
class V2NotifyCallbacks : public BLECharacteristicCallbacks {
  void onStatus(BLECharacteristic*, Status status, uint32_t code) override {
    v2LastNotifyStatus = int(status);
    v2LastNotifyCode = code;
    portENTER_CRITICAL(&v2DiagnosticMux);
    v2CurrentDiagnostic.notifyStatus = int(status);
    v2CurrentDiagnostic.notifyCode = code;
    portEXIT_CRITICAL(&v2DiagnosticMux);
    v2NotifyAccepted = status == SUCCESS_NOTIFY;
    if (!v2NotifyAccepted)
      bleLogf("[V2] Notify submit failed status=%d code=%lu\n", int(status), (unsigned long)code);
  }
};

void initV2Gatt() {
  v2Inbound = xQueueCreate(32, sizeof(V2Inbound));
  v2JobMutex = xSemaphoreCreateMutex();
  v2RequestIds = static_cast<double*>(ps_malloc(16384 * sizeof(double)));
  if (!v2Inbound || !v2JobMutex || !v2RequestIds) { Serial.println("[V2] Allocation/PSRAM failed; v2 disabled."); return; }
  snprintf(v2Boot, sizeof(v2Boot), "b%08lx%08lx", (unsigned long)esp_random(), (unsigned long)esp_random());
  BLEService* service = bleServer->createService(BLEUUID(V2_SERVICE), 20);
  BLECharacteristic* command = service->createCharacteristic(V2_COMMAND, BLECharacteristic::PROPERTY_WRITE);
  command->setCallbacks(new V2WriteCallbacks());
  v2Response = service->createCharacteristic(V2_RESPONSE, BLECharacteristic::PROPERTY_NOTIFY);
  v2Response->setCallbacks(new V2NotifyCallbacks());
  v2Cccd = new BLE2902();
  v2Cccd->setCallbacks(new V2SubscriptionCallbacks());
  v2Response->addDescriptor(v2Cccd);
  initBulkGatt(service);
  service->start();
  // Keep the legacy advertisement: two 128-bit UUIDs do not fit one legacy packet.
  bleLogf("[V2] Service ready; boot %s i2s=%u sd=%u buffer=%u\n", v2Boot,
          unsigned(i2sReady), unsigned(sdReady), unsigned(audioBuffer != nullptr));
}

V2Job v2Snapshot() {
  xSemaphoreTake(v2JobMutex, portMAX_DELAY);
  V2Job copy = v2Job;
  xSemaphoreGive(v2JobMutex);
  return copy;
}
void v2SetPhase(V2Phase phase, uint8_t sensor) {
  xSemaphoreTake(v2JobMutex, portMAX_DELAY);
  v2Job.phase = phase; v2Job.sensor = sensor;
  xSemaphoreGive(v2JobMutex);
}
bool v2Cancelled(uint32_t epoch) {
  return v2Stop || !bleDeviceConnected || !v2Subscribed || v2Epoch != epoch;
}
bool v2Idle() {
  return !v2OwnsRecorder && !v2BulkBusy && !recording && !recordingTaskRunning && !scanningAll && !bleCapture;
}
bool v2Ready() { return v2Idle() && i2sReady && sdReady && audioBuffer; }

// Verify closed bytes against the exact bytes written, not merely a proposed size.
struct V2HeaderSink {
  uint8_t bytes[44];
  size_t used = 0;
  size_t write(const uint8_t* data, size_t count) {
    if (used + count > sizeof(bytes)) return 0;
    memcpy(bytes + used, data, count); used += count; return count;
  }
};
bool v2Save(V2File& meta) {
  if (SD.exists(meta.path)) return false;
  File out = SD.open(meta.path, FILE_WRITE);
  if (!out) return false;
  V2HeaderSink header;
  writeWavHeader(header, meta.samples * 2);
  bool good = header.used == 44 && out.write(header.bytes, 44) == 44;
  uint32_t expected = BleV2Wire::crcUpdate(0xffffffffU, header.bytes, 44);
  const uint8_t* pcm = reinterpret_cast<const uint8_t*>(audioBuffer);
  size_t remaining = meta.samples * 2;
  while (good && remaining) {
    size_t count = remaining > 1024 ? 1024 : remaining;
    good = out.write(pcm, count) == count;
    expected = BleV2Wire::crcUpdate(expected, pcm, count);
    pcm += count; remaining -= count;
    vTaskDelay(1);
  }
  out.flush();
  if (out.getWriteError()) good = false;
  out.close();
  if (!good) return false;
  File in = SD.open(meta.path, FILE_READ);
  meta.bytes = 44 + meta.samples * 2;
  if (!in || in.size() != meta.bytes) { in.close(); return false; }
  uint8_t block[1024];
  uint32_t crc = 0xffffffffU;
  remaining = meta.bytes;
  while (remaining) {
    size_t count = remaining > sizeof(block) ? sizeof(block) : remaining;
    if (in.read(block, count) != count) { in.close(); return false; }
    crc = BleV2Wire::crcUpdate(crc, block, count);
    remaining -= count;
    vTaskDelay(1);
  }
  in.close();
  meta.crc = crc ^ 0xffffffffU;
  return meta.crc == (expected ^ 0xffffffffU);
}

void v2CaptureTask(void*) {
  V2Job plan = v2Snapshot();
  V2Phase terminal = V2Phase::Complete;
  uint8_t first = plan.scan ? 1 : plan.sensor;
  uint8_t last = plan.scan ? 8 : plan.sensor;
  for (uint8_t sensor = first; sensor <= last; ++sensor) {
    if (v2Cancelled(plan.epoch)) { terminal = V2Phase::Cancelled; break; }
    v2SetPhase(V2Phase::Selecting, sensor);
    selectMicrophone(sensor);
    if (v2Cancelled(plan.epoch)) { terminal = V2Phase::Cancelled; break; }
    if (i2s_zero_dma_buffer(I2S_PORT) != ESP_OK) { terminal = V2Phase::Failed; break; }
    resetFilters();
    sampleCount = 0;
    v2SetPhase(V2Phase::Recording, sensor);
    uint32_t started = millis();
    bool readError = false;
    while (sampleCount < MAX_SAMPLES && millis() - started < 20000 && !v2Cancelled(plan.epoch)) {
      int32_t dma[128];
      size_t bytes = 0;
      esp_err_t error = i2s_read(I2S_PORT, dma, sizeof(dma), &bytes, pdMS_TO_TICKS(100));
      if (error != ESP_OK && error != ESP_ERR_TIMEOUT) { readError = true; break; }
      if (bytes % sizeof(int32_t)) { readError = true; break; }
      for (size_t i = 0; i < bytes / sizeof(int32_t) && sampleCount < MAX_SAMPLES; ++i) {
        audioBuffer[sampleCount++] = processAudioSample(dma[i]);
      }
      vTaskDelay(1);
    }
    if (readError || !sampleCount) {
      terminal = v2Cancelled(plan.epoch) ? V2Phase::Cancelled : V2Phase::Failed;
      break;
    }
    v2SetPhase(V2Phase::Finalizing, sensor);
    V2File meta = {};
    meta.sensor = sensor; meta.samples = sampleCount;
    uint32_t serial = v2NextCapture++;
    snprintf(meta.captureId, sizeof(meta.captureId), "c%lu", (unsigned long)serial);
    snprintf(meta.fileId, sizeof(meta.fileId), "f%lu", (unsigned long)serial);
    snprintf(meta.path, sizeof(meta.path), "/BLEV2_%s_%s_%s.wav", v2Boot, plan.id, meta.fileId);
    if (!v2Save(meta)) { sdReady = false; terminal = V2Phase::Failed; break; }
    xSemaphoreTake(v2JobMutex, portMAX_DELAY);
    v2Job.files[v2Job.count++] = meta;
    xSemaphoreGive(v2JobMutex);
    Serial.printf("[V2] Finalized %s sensor %u: %lu samples, CRC %08lx\n",
                  meta.fileId, sensor, (unsigned long)meta.samples, (unsigned long)meta.crc);
    if (v2Cancelled(plan.epoch)) { terminal = V2Phase::Partial; break; }
    if (sensor < last) {
      v2SetPhase(V2Phase::Resting, sensor);
      uint32_t rest = millis();
      while (millis() - rest < 5000 && !v2Cancelled(plan.epoch)) vTaskDelay(pdMS_TO_TICKS(10));
      if (v2Cancelled(plan.epoch)) { terminal = V2Phase::Partial; break; }
    }
  }
  xSemaphoreTake(v2JobMutex, portMAX_DELAY);
  // Serialize STOP vs terminal publication; a stopped job cannot become complete.
  if (terminal == V2Phase::Complete && v2Cancelled(plan.epoch)) terminal = V2Phase::Partial;
  v2Job.phase = terminal;
  v2OwnsRecorder = false;
  xSemaphoreGive(v2JobMutex);
  Serial.printf("[V2] Job %s terminal phase %d\n", plan.id, int(terminal));
  vTaskDelete(nullptr);
}

const char* v2PhaseName(V2Phase phase) {
  switch (phase) {
    case V2Phase::Selecting: return "selecting";
    case V2Phase::Recording: return "recording";
    case V2Phase::Finalizing: return "finalizing";
    case V2Phase::Resting: return "resting";
    case V2Phase::Complete: return "complete";
    case V2Phase::Partial: return "partial";
    case V2Phase::Cancelled: return "cancelled";
    default: return "failed";
  }
}

bool v2Integer(cJSON* object, const char* key, double low, double high, double& number) {
  cJSON* item = cJSON_GetObjectItemCaseSensitive(object, key);
  if (!cJSON_IsNumber(item)) return false;
  number = item->valuedouble;
  return isfinite(number) && number >= low && number <= high && floor(number) == number;
}
const char* v2String(cJSON* object, const char* key) {
  cJSON* item = cJSON_GetObjectItemCaseSensitive(object, key);
  return cJSON_IsString(item) ? item->valuestring : "";
}
cJSON* v2Result() {
  cJSON* result = cJSON_CreateObject();
  cJSON_AddStringToObject(result, "bootId", v2Boot);
  return result;
}
#include "AuscultaBleBulk.h"
void v2Reply(double id, cJSON* result, const char* error = nullptr) {
  cJSON* response = cJSON_CreateObject();
  cJSON_AddNumberToObject(response, "id", id);
  cJSON_AddBoolToObject(response, "ok", error == nullptr);
  if (error) {
    cJSON_Delete(result);
    cJSON_AddStringToObject(response, "error", error);
  } else cJSON_AddItemToObject(response, "result", result);
  char* json = cJSON_PrintUnformatted(response);
  if (json) { v2Tx = json; cJSON_free(json); }
  else v2Fail("response_allocation");
  cJSON_Delete(response);
  if (!v2Tx.length() || v2Tx.length() > 4096) { v2Fail("response_length"); v2Tx = ""; }
  v2TxOffset = 0; v2TxStarted = millis();
  v2SendWindow.reset();
  portENTER_CRITICAL(&v2DiagnosticMux);
  v2CurrentDiagnostic.responseOffset = 0;
  v2CurrentDiagnostic.responseLength = v2Tx.length();
  portEXIT_CRITICAL(&v2DiagnosticMux);
}

void v2HandleRequest(cJSON* request) {
  double id;
  if (!v2Integer(request, "id", 1, 9007199254740991.0, id)) { v2Fail("invalid_request_id"); return; }
  // IDs are unique within a link. Reject duplicates without executing them again.
  for (size_t i = 0; i < v2RequestCount; ++i) if (v2RequestIds[i] == id) { v2Fail("duplicate_request_id"); return; }
  if (v2RequestCount >= 16384) { v2Fail("request_history_full"); return; }
  v2RequestIds[v2RequestCount++] = id;
  const char* cmd = v2String(request, "cmd");
  uint32_t stackFree = uxTaskGetStackHighWaterMark(nullptr);
  double diagnosticOffset = 0;
  v2Integer(request, "offset", 0, 0xffffffffU, diagnosticOffset);
  portENTER_CRITICAL(&v2DiagnosticMux);
  v2CurrentDiagnostic.requestId = id;
  strlcpy(v2CurrentDiagnostic.command, cmd, sizeof(v2CurrentDiagnostic.command));
  strlcpy(v2CurrentDiagnostic.fileId, v2String(request, "fileId"), sizeof(v2CurrentDiagnostic.fileId));
  v2CurrentDiagnostic.fileOffset = uint32_t(diagnosticOffset);
  v2CurrentDiagnostic.stackFree = stackFree;
  v2CurrentDiagnostic.notifyStatus = -1;
  v2CurrentDiagnostic.notifyCode = 0;
  v2CurrentDiagnostic.responseOffset = v2CurrentDiagnostic.responseLength = 0;
  portEXIT_CRITICAL(&v2DiagnosticMux);
  if (strcmp(cmd, "GET_JOB") && strcmp(cmd, "READ_FILE"))
    bleLogf("[V2] Command: %s id=%.0f loopStackFree=%u\n", cmd, id,
            unsigned(uxTaskGetStackHighWaterMark(nullptr)));
  const char* error = nullptr;
  cJSON* result = v2Result();
  if (!strcmp(cmd, "GET_CAPABILITIES")) {
    v2CapabilitiesKnown = true;
    cJSON_AddNumberToObject(result, "version", 2);
    cJSON_AddBoolToObject(result, "ready", v2Ready());
    cJSON_AddNumberToObject(result, "sampleRate", SAMPLE_RATE);
    cJSON_AddStringToObject(result, "format", "wav-pcm16-mono");
    int sensors[] = {1,2,3,4,5,6,7,8};
    cJSON_AddItemToObject(result, "sensorIds", cJSON_CreateIntArray(sensors, 8));
    const char* modes[] = {"single", "scan"};
    cJSON_AddItemToObject(result, "modes", cJSON_CreateStringArray(modes, 2));
    bulkCapabilities(result);
    cJSON* diagnostic = cJSON_AddObjectToObject(result, "diagnostics");
    cJSON_AddStringToObject(diagnostic, "build", "v2-bulk-timing-3");
    cJSON_AddNumberToObject(diagnostic, "notificationRetriesSinceBoot", v2NotifyRetries);
    V2LinkDiagnostic last;
    portENTER_CRITICAL(&v2DiagnosticMux);
    last = v2LastDisconnect;
    portEXIT_CRITICAL(&v2DiagnosticMux);
    if (!last.valid) cJSON_AddNullToObject(diagnostic, "lastDisconnect");
    else {
      cJSON* previous = cJSON_AddObjectToObject(diagnostic, "lastDisconnect");
      cJSON_AddNumberToObject(previous, "atMs", last.atMs);
      cJSON_AddBoolToObject(previous, "firmwareFault", last.localFault);
      cJSON_AddStringToObject(previous, "fault", last.fault);
      cJSON_AddNumberToObject(previous, "disconnectReason", last.disconnectReason);
      cJSON_AddNumberToObject(previous, "requestId", last.requestId);
      cJSON_AddStringToObject(previous, "command", last.command);
      cJSON_AddStringToObject(previous, "fileId", last.fileId);
      cJSON_AddNumberToObject(previous, "fileOffset", last.fileOffset);
      cJSON_AddNumberToObject(previous, "responseOffset", last.responseOffset);
      cJSON_AddNumberToObject(previous, "responseLength", last.responseLength);
      cJSON_AddNumberToObject(previous, "notifyStatus", last.notifyStatus);
      cJSON_AddNumberToObject(previous, "notifyCode", last.notifyCode);
      cJSON_AddNumberToObject(previous, "freeHeap", last.freeHeap);
      cJSON_AddNumberToObject(previous, "minHeap", last.minHeap);
      cJSON_AddNumberToObject(previous, "loopStackFree", last.stackFree);
    }
  } else if (!v2CapabilitiesKnown) error = "NOT_READY";
  else if (strcmp(v2String(request, "bootId"), v2Boot)) error = "STALE_BOOT";
  else if (!strcmp(cmd, "SELECT_SENSOR")) {
    double sensor;
    if (!v2Integer(request, "sensorId", 1, 8, sensor)) error = "BAD_REQUEST";
    else if (!v2Idle()) error = "BUSY";
    else if (!v2Ready()) error = "NOT_READY";
    else {
      selectMicrophone(uint8_t(sensor));
      v2ConfirmedSensor = uint8_t(sensor);
      cJSON_AddNumberToObject(result, "sensorId", selectedMic);
    }
  } else if (!strcmp(cmd, "START_CAPTURE") || !strcmp(cmd, "START_SCAN")) {
    bool scan = !strcmp(cmd, "START_SCAN");
    double sensor = 1;
    if (!scan && !v2Integer(request, "sensorId", 1, 8, sensor)) error = "BAD_REQUEST";
    else if (!v2Idle()) error = "BUSY";
    else if (!v2Ready()) error = "NOT_READY";
    else if (!scan && (v2ConfirmedSensor != sensor || selectedMic != sensor)) error = "SENSOR_MISMATCH";
    else {
      V2Job job = {};
      job.epoch = v2LocalEpoch; job.scan = scan; job.sensor = uint8_t(sensor);
      job.phase = V2Phase::Selecting;
      snprintf(job.id, sizeof(job.id), "j%lu", (unsigned long)v2NextJob++);
      xSemaphoreTake(v2JobMutex, portMAX_DELAY);
      v2Job = job; v2Stop = false; v2OwnsRecorder = true;
      xSemaphoreGive(v2JobMutex);
      if (xTaskCreatePinnedToCore(v2CaptureTask, "V2Capture", 8192, nullptr, 3, nullptr, 0) != pdPASS) {
        v2SetPhase(V2Phase::Failed, job.sensor); v2OwnsRecorder = false;
        error = "CAPTURE_FAILED";
      } else {
        cJSON_AddStringToObject(result, "jobId", job.id);
        Serial.printf("[V2] Accepted %s job %s\n", cmd, job.id);
      }
    }
  } else if (!strcmp(cmd, "GET_JOB") || !strcmp(cmd, "STOP_JOB")) {
    V2Job job = v2Snapshot();
    if (job.phase == V2Phase::None || job.epoch != v2Epoch || strcmp(v2String(request, "jobId"), job.id)) error = "NOT_FOUND";
    else if (!strcmp(cmd, "STOP_JOB")) {
      xSemaphoreTake(v2JobMutex, portMAX_DELAY);
      if (!v2OwnsRecorder) error = "BAD_REQUEST";
      else v2Stop = true;
      xSemaphoreGive(v2JobMutex);
      cJSON_AddStringToObject(result, "jobId", job.id);
      cJSON_AddStringToObject(result, "phase", "stop_requested");
    } else {
      cJSON_AddStringToObject(result, "jobId", job.id);
      cJSON_AddStringToObject(result, "phase", v2PhaseName(job.phase));
      if (job.phase == V2Phase::Selecting || job.phase == V2Phase::Recording || job.phase == V2Phase::Finalizing || job.phase == V2Phase::Resting)
        cJSON_AddNumberToObject(result, "sensorId", job.sensor);
      if (job.phase == V2Phase::Complete) {
        cJSON* files = cJSON_AddArrayToObject(result, "files");
        for (uint8_t i = 0; i < job.count; ++i) {
          const V2File& meta = job.files[i];
          cJSON* file = cJSON_CreateObject();
          cJSON_AddStringToObject(file, "jobId", job.id);
          cJSON_AddStringToObject(file, "captureId", meta.captureId);
          cJSON_AddStringToObject(file, "fileId", meta.fileId);
          cJSON_AddNumberToObject(file, "sensorId", meta.sensor);
          cJSON_AddNumberToObject(file, "sampleCount", meta.samples);
          cJSON_AddNumberToObject(file, "byteLength", meta.bytes);
          char crc[9]; snprintf(crc, sizeof(crc), "%08lx", (unsigned long)meta.crc);
          cJSON_AddStringToObject(file, "crc32", crc);
          cJSON_AddItemToArray(files, file);
        }
      }
    }
  } else if (!strcmp(cmd, "READ_FILE")) {
    V2Job job = v2Snapshot();
    const V2File* meta = nullptr;
    if (job.epoch == v2Epoch && job.phase == V2Phase::Complete)
      for (uint8_t i = 0; i < job.count; ++i)
        if (!strcmp(v2String(request, "fileId"), job.files[i].fileId)) meta = &job.files[i];
    double offset = 0, length = 0;
    if (!meta) error = "NOT_FOUND";
    else if (!v2Integer(request, "offset", 0, meta->bytes, offset) ||
             !v2Integer(request, "length", 1, 768, length) || offset + length > meta->bytes) error = "RANGE";
    else if (!v2Idle()) error = "BUSY";
    else {
      uint8_t bytes[768];
      File in = SD.open(meta->path, FILE_READ);
      bool good = in && in.size() == meta->bytes && in.seek(uint32_t(offset)) && in.read(bytes, size_t(length)) == size_t(length);
      in.close();
      if (!good) { sdReady = false; error = "CAPTURE_FAILED"; }
      else {
        unsigned char base64[1025]; size_t written = 0;
        if (mbedtls_base64_encode(base64, sizeof(base64), &written, bytes, size_t(length))) error = "CAPTURE_FAILED";
        else {
          base64[written] = 0;
          cJSON_AddStringToObject(result, "fileId", meta->fileId);
          cJSON_AddNumberToObject(result, "offset", offset);
          cJSON_AddStringToObject(result, "data", reinterpret_cast<char*>(base64));
        }
      }
    }
  } else if (!strncmp(cmd, "BULK_", 5)) error = bulkCommand(cmd, request, result);
  else error = *cmd ? "UNSUPPORTED" : "BAD_REQUEST";
  v2Reply(id, result, error);
}

void processV2Bluetooth() {
  if (!v2Response) return;
  if (v2LocalEpoch != v2Epoch) {
    if (v2BulkBusy && bulk.epoch != v2Epoch) bulkClose("cancelled", "LINK_RETIRED");
    v2LocalEpoch = v2Epoch;
    v2Rx.reset(); v2Tx = ""; v2TxOffset = 0;
    v2ConfirmedSensor = 0; v2CapabilitiesKnown = false;
    v2RequestCount = 0;
  }
  if (v2Fault) {
    bleLogf("[V2] Retiring link: reason=%s assembled=%u/%u txOffset=%u epoch=%lu\n",
            v2FaultReason, unsigned(v2Rx.used), unsigned(v2Rx.total), unsigned(v2TxOffset),
            (unsigned long)v2Epoch);
    v2Fault = false;
    retireV2Link();
    if (bleDeviceConnected) bleServer->disconnect(bleServer->getConnId());
    return;
  }
  if (!bleDeviceConnected || !v2Subscribed) return;
  if (v2Rx.used && millis() - v2LastRxAt > 4000) { v2Fail("assembly_timeout"); return; }
  if (v2Tx.length()) {
    if (millis() - v2TxStarted > 3500) { v2Fail("response_timeout"); return; }
    // Do not flood the NimBLE mbuf pool. A rejected ENOMEM submission has
    // not been queued; retain the exact fragment and wait before resubmitting.
    if (!v2SendWindow.ready(millis())) return;
    uint8_t frame[20];
    size_t count = BleV2Wire::fragment(reinterpret_cast<const uint8_t*>(v2Tx.c_str()), v2Tx.length(), v2TxOffset, frame);
    v2NotifyAccepted = false;
    v2LastNotifyStatus = -1;
    v2LastNotifyCode = 0;
    v2Response->setValue(frame, count);
    v2Response->notify();
    if (!v2NotifyAccepted) {
      bool noMemory = false;
      if (v2LastNotifyStatus == int(BLECharacteristicCallbacks::ERROR_GATT)) {
#if defined(CONFIG_NIMBLE_ENABLED)
        noMemory = v2LastNotifyCode == BLE_HS_ENOMEM;
#elif defined(CONFIG_BLUEDROID_ENABLED)
        noMemory = v2LastNotifyCode == ESP_ERR_NO_MEM;
#endif
      }
      if (noMemory) {
        ++v2NotifyRetries;
        if (v2SendWindow.noMemory(millis())) return;
        v2Fail("notification_memory_retries_exhausted");
      } else v2Fail("notification_submit");
      return;
    }
    v2SendWindow.accepted(millis());
    v2TxOffset += count - 4;
    portENTER_CRITICAL(&v2DiagnosticMux);
    v2CurrentDiagnostic.responseOffset = v2TxOffset;
    portEXIT_CRITICAL(&v2DiagnosticMux);
    v2LastTxAt = millis();
    if (v2TxOffset == v2Tx.length()) v2Tx = "";
    return;
  }
  V2Inbound frame;
  while (xQueueReceive(v2Inbound, &frame, 0) == pdTRUE) {
    if (frame.epoch != v2Epoch) continue;
    v2LastRxAt = millis();
    uint16_t previous = v2Rx.used;
    BleV2Wire::Feed status = v2Rx.add(frame.bytes, frame.size);
    if (v2TraceBudget)
      bleLogf("[V2] RX before=%u after=%u total=%u result=%d\n", unsigned(previous),
              unsigned(v2Rx.used), unsigned(v2Rx.total), int(status));
    if (status == BleV2Wire::Feed::Invalid) { v2Fail("invalid_fragment"); return; }
    if (status == BleV2Wire::Feed::Complete) {
      // Embedded NUL or trailing non-JSON bytes are never accepted.
      cJSON* request = strlen(v2Rx.bytes) == v2Rx.total
        ? cJSON_ParseWithLengthOpts(v2Rx.bytes, v2Rx.total + 1, nullptr, true) : nullptr;
      if (!cJSON_IsObject(request)) v2Fail("invalid_json");
      else v2HandleRequest(request);
      cJSON_Delete(request); v2Rx.reset();
      return;
    }
  }
}

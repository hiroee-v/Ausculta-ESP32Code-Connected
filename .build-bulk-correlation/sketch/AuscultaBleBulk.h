#line 1 "C:\\Users\\Jerome Vargas\\Downloads\\Ausculta_Recorder_with_SD_Card_Module_All_8_Mics_WORKING__1_\\AuscultaBleBulk.h"
#pragma once
#include "BleBulkWire.h"
#if defined(CONFIG_NIMBLE_ENABLED)
#include <host/ble_gatt.h>
#include <host/ble_hs_mbuf.h>
#endif

#define BULK_DATA_UUID "607d9703-6764-4cc9-b134-70e3c04c8500"
#define BULK_ACK_UUID  "607d9704-6764-4cc9-b134-70e3c04c8500"

struct BulkAckFrame { uint32_t epoch; uint8_t bytes[16]; };
struct BulkTransfer {
  uint32_t handle = 0, epoch = 0, startedAt = 0, endedAt = 0, deadlineAt = 0, lastSendAt = 0;
  uint32_t notifications = 0, memoryRetries = 0;
  uint16_t valueLimit = 0, mtu = 0, pendingBytes = 0;
  uint8_t failures = 0;
  char jobId[24] = {};
  V2File meta = {};
  const char* state = "idle";
  const char* error = "";
  bool started = false;
  BleBulkWire::Window window;
  File file;
  uint8_t pending[BleBulkWire::MaxValueBytes];
};
BulkTransfer bulk;
struct BulkTiming {
  uint64_t sdReadUs=0, pacingWaitUs=0, ackCreditWaitUs=0, notifySubmitUs=0, txWaitUs=0;
  uint32_t attempts=0, noMemory=0, otherRejects=0;
  int lastSubmitCode=0;
};
BulkTiming bulkTiming;
portMUX_TYPE bulkTxMux = portMUX_INITIALIZER_UNLOCKED;
uint32_t bulkTxAccepted=0, bulkTxCompleted=0, bulkTxErrors=0;
int bulkTxLastError=0;
bool bulkTxEnabled=false;
BleBulkWire::TxTracker bulkTracker;
uint64_t bulkWaitSince=0;
enum class BulkWait { None, Pacing, Ack, Tx };
BulkWait bulkWaitKind=BulkWait::None;
void bulkWait(BulkWait kind) {
  uint64_t now=esp_timer_get_time();
  if (bulkWaitSince) {
    uint64_t elapsed=now-bulkWaitSince;
    if (bulkWaitKind==BulkWait::Pacing) bulkTiming.pacingWaitUs+=elapsed;
    if (bulkWaitKind==BulkWait::Ack) bulkTiming.ackCreditWaitUs+=elapsed;
    if (bulkWaitKind==BulkWait::Tx) bulkTiming.txWaitUs+=elapsed;
  }
  bulkWaitKind=kind; bulkWaitSince=kind==BulkWait::None?0:now;
}
uint32_t bulkNextHandle = 1;
QueueHandle_t bulkAckQueue = nullptr;
BLECharacteristic* bulkData = nullptr;
BLE2902* bulkCccd = nullptr;
volatile bool bulkBadAck = false;
volatile int bulkNotifyStatus = -1;
volatile uint32_t bulkNotifyCode = 0;

class BulkAckCallbacks : public BLECharacteristicCallbacks {
  void onWrite(BLECharacteristic* characteristic) override {
    auto value = characteristic->getValue();
    if (!bleDeviceConnected || !v2Subscribed || !bulkCccd->getNotifications()) return;
    if (value.length() != 16) { bulkBadAck = true; return; }
    BulkAckFrame frame;
    frame.epoch = v2Epoch;
    memcpy(frame.bytes, value.c_str(), 16);
    if (xQueueSend(bulkAckQueue, &frame, 0) != pdTRUE) bulkBadAck = true;
  }
};
class BulkDataCallbacks : public BLECharacteristicCallbacks {
  void onStatus(BLECharacteristic*, Status status, uint32_t code) override {
#if defined(CONFIG_NIMBLE_ENABLED)
    // Native API below bypasses BLECharacteristic::notify's immediate callbacks.
    // These callbacks are now exclusively BLE_GAP_EVENT_NOTIFY_TX events.
    portENTER_CRITICAL(&bulkTxMux);
    if (bulkTxEnabled) {
      bulkTracker.event(status==SUCCESS_NOTIFY?0:int(code?code:0xffffffffU));
      bulkTxCompleted=bulkTracker.completed;
      bulkTxErrors=bulkTracker.errors; bulkTxLastError=bulkTracker.lastError;
    }
    portEXIT_CRITICAL(&bulkTxMux);
#else
    bulkNotifyStatus = int(status); bulkNotifyCode = code;
#endif
  }
};
void initBulkGatt(BLEService* service) {
  bulkAckQueue = xQueueCreate(8, sizeof(BulkAckFrame));
  if (!bulkAckQueue) return; // Old app and READ_FILE remain usable.
  bulkData = service->createCharacteristic(BULK_DATA_UUID, BLECharacteristic::PROPERTY_NOTIFY);
  bulkData->setCallbacks(new BulkDataCallbacks());
  bulkCccd = new BLE2902();
  bulkData->addDescriptor(bulkCccd);
  BLECharacteristic* ack = service->createCharacteristic(BULK_ACK_UUID, BLECharacteristic::PROPERTY_WRITE);
  ack->setCallbacks(new BulkAckCallbacks());
}
void bulkClose(const char* state, const char* error = "") {
  bulkWait(BulkWait::None);
  bulk.file.close(); bulk.pendingBytes = 0;
  bulk.state = state; bulk.error = error; bulk.endedAt = millis();
  v2BulkBusy = false;
}
void bulkCapabilities(cJSON* result) {
  if (!bulkData) return;
  cJSON* capability = cJSON_AddObjectToObject(result, "bulkDownload");
  cJSON_AddNumberToObject(capability, "version", 1);
  cJSON_AddStringToObject(capability, "dataUuid", BULK_DATA_UUID);
  cJSON_AddStringToObject(capability, "ackUuid", BULK_ACK_UUID);
  cJSON_AddNumberToObject(capability, "headerBytes", BleBulkWire::HeaderBytes);
  cJSON_AddNumberToObject(capability, "maxWindowPackets", BleBulkWire::MaxWindow);
  cJSON_AddNumberToObject(capability, "ackTimeoutMs", 10000);
}
void bulkStatus(cJSON* result) {
  cJSON_AddNumberToObject(result, "transferId", bulk.handle);
  cJSON_AddStringToObject(result, "jobId", bulk.jobId);
  cJSON_AddStringToObject(result, "captureId", bulk.meta.captureId);
  cJSON_AddStringToObject(result, "fileId", bulk.meta.fileId);
  cJSON_AddNumberToObject(result, "sensorId", bulk.meta.sensor);
  cJSON_AddNumberToObject(result, "byteLength", bulk.meta.bytes);
  cJSON_AddNumberToObject(result, "sampleCount", bulk.meta.samples);
  char crc[9]; snprintf(crc, sizeof(crc), "%08lx", (unsigned long)bulk.meta.crc);
  cJSON_AddStringToObject(result, "crc32", crc);
  cJSON_AddStringToObject(result, "state", bulk.state);
  cJSON_AddStringToObject(result, "error", bulk.error);
  cJSON_AddNumberToObject(result, "mtu", bulk.mtu);
  cJSON_AddNumberToObject(result, "valueBytes", bulk.valueLimit);
  cJSON_AddNumberToObject(result, "payloadBytes", bulk.valueLimit - BleBulkWire::HeaderBytes);
  cJSON_AddNumberToObject(result, "submittedBytes", bulk.window.sent);
  cJSON_AddNumberToObject(result, "acknowledgedBytes", bulk.window.acknowledged);
  cJSON_AddNumberToObject(result, "notifications", bulk.notifications);
  cJSON_AddNumberToObject(result, "memoryRetries", bulk.memoryRetries);
  cJSON* timing=cJSON_AddObjectToObject(result,"timing");
  cJSON_AddNumberToObject(timing,"sdReadUs",double(bulkTiming.sdReadUs));
  cJSON_AddNumberToObject(timing,"pacingWaitUs",double(bulkTiming.pacingWaitUs));
  cJSON_AddNumberToObject(timing,"ackCreditWaitUs",double(bulkTiming.ackCreditWaitUs));
  cJSON_AddNumberToObject(timing,"txWaitUs",double(bulkTiming.txWaitUs));
  cJSON_AddNumberToObject(timing,"notifySubmitUs",double(bulkTiming.notifySubmitUs));
  cJSON_AddNumberToObject(timing,"submitAttempts",bulkTiming.attempts);
  cJSON_AddNumberToObject(timing,"rejectedNoMemory",bulkTiming.noMemory);
  cJSON_AddNumberToObject(timing,"rejectedOther",bulkTiming.otherRejects);
  cJSON_AddNumberToObject(timing,"lastSubmitCode",bulkTiming.lastSubmitCode);
  portENTER_CRITICAL(&bulkTxMux);
  uint32_t accepted=bulkTxAccepted, completed=bulkTxCompleted, errors=bulkTxErrors;
  int txError=bulkTxLastError;
  uint32_t rejectedCallbacks=bulkTracker.rejectedCallbacks, unownedCallbacks=bulkTracker.unownedCallbacks;
  uint32_t callbackAttempt=bulkTracker.callbackAttempt;
  portEXIT_CRITICAL(&bulkTxMux);
#if defined(CONFIG_NIMBLE_ENABLED)
  cJSON_AddBoolToObject(timing,"nativeTxEventsAvailable",true);
#else
  cJSON_AddBoolToObject(timing,"nativeTxEventsAvailable",false);
#endif
  cJSON_AddNumberToObject(timing,"nativeTxEvents",completed);
  cJSON_AddNumberToObject(timing,"nativeTxErrors",errors);
  cJSON_AddNumberToObject(timing,"lastNativeTxError",txError);
  cJSON_AddNumberToObject(timing,"outstandingSubmissions",accepted>completed?accepted-completed:0);
  cJSON_AddNumberToObject(timing,"rejectedSubmissionCallbacks",rejectedCallbacks);
  cJSON_AddNumberToObject(timing,"unownedTxCallbacks",unownedCallbacks);
  cJSON_AddNumberToObject(timing,"lastCallbackAttempt",callbackAttempt);
  uint32_t elapsed = bulk.started ? (v2BulkBusy ? millis() : bulk.endedAt) - bulk.startedAt : 0;
  cJSON_AddNumberToObject(result, "elapsedMs", elapsed);
  cJSON_AddNumberToObject(result, "acknowledgedBytesPerSecond", elapsed ? double(bulk.window.acknowledged) * 1000 / elapsed : 0);
}
const char* bulkCommand(const char* cmd, cJSON* request, cJSON* result) {
  if (!bulkData) return "UNSUPPORTED";
  if (!strcmp(cmd, "BULK_OPEN")) {
    if (!v2Idle()) return "BUSY";
    portENTER_CRITICAL(&bulkTxMux);
    bool outstanding=bulkTxAccepted>bulkTxCompleted;
    portEXIT_CRITICAL(&bulkTxMux);
#if defined(CONFIG_NIMBLE_ENABLED)
    if (outstanding) return "BUSY";
#endif
    if (!bulkCccd->getNotifications()) return "NOT_READY";
    uint16_t mtu = bleServer->getPeerMTU(bleServer->getConnId());
    uint16_t limit = BleBulkWire::valueBytes(mtu);
    if (limit <= BleBulkWire::HeaderBytes || !bulkNextHandle) return "NOT_READY";
    double sensor;
    if (!v2Integer(request, "sensorId", 1, 8, sensor)) return "BAD_REQUEST";
    V2File meta = {}; char jobId[24] = {}; bool found = false;
    const char* fileId = v2String(request, "fileId");
    xSemaphoreTake(v2JobMutex, portMAX_DELAY);
    if (v2Job.epoch == v2Epoch && v2Job.phase == V2Phase::Complete) {
      for (uint8_t i = 0; i < v2Job.count; ++i) {
        if (!strcmp(fileId, v2Job.files[i].fileId)) {
          meta = v2Job.files[i]; strlcpy(jobId, v2Job.id, sizeof(jobId)); found = true; break;
        }
      }
    }
    xSemaphoreGive(v2JobMutex);
    if (!found) return "NOT_FOUND";
    if (sensor != meta.sensor) return "SENSOR_MISMATCH";
    bulk.file.close();
    bulk.file = SD.open(meta.path, FILE_READ);
    if (!bulk.file || bulk.file.size() != meta.bytes) { bulk.file.close(); return "CAPTURE_FAILED"; }
    bulk.handle = bulkNextHandle++; bulk.epoch = v2Epoch;
    bulkTiming=BulkTiming{}; bulkWaitSince=0; bulkWaitKind=BulkWait::None;
    portENTER_CRITICAL(&bulkTxMux);
    bulkTxAccepted=bulkTxCompleted=bulkTxErrors=0; bulkTxLastError=0; bulkTxEnabled=true;
    bulkTracker=BleBulkWire::TxTracker{};
    portEXIT_CRITICAL(&bulkTxMux);
    bulk.meta = meta; strlcpy(bulk.jobId, jobId, sizeof(bulk.jobId));
    bulk.window.reset(meta.bytes); bulk.mtu = mtu; bulk.valueLimit = limit;
    bulk.notifications = bulk.memoryRetries = 0; bulk.pendingBytes = bulk.failures = 0;
    bulk.started = false; bulk.startedAt = bulk.endedAt = 0;
    bulk.lastSendAt = 0; bulk.deadlineAt = millis();
    bulk.state = "awaiting_ack"; bulk.error = "";
    xQueueReset(bulkAckQueue); bulkBadAck = false;
    v2BulkBusy = true;
    bulkStatus(result); // No binary data until the phone grants its first window.
    return nullptr;
  }
  if (strcmp(cmd, "BULK_STATUS") && strcmp(cmd, "BULK_CANCEL")) return "UNSUPPORTED";
  double handle;
  if (!v2Integer(request, "transferId", 1, 0xffffffffU, handle)) return "BAD_REQUEST";
  if (bulk.epoch != v2Epoch || handle != bulk.handle) return "NOT_FOUND";
  if (!strcmp(cmd, "BULK_CANCEL") && v2BulkBusy) bulkClose("cancelled", "USER_CANCELLED");
  bulkStatus(result); return nullptr;
}

void processBulkBluetooth() {
  if (!bulkData) return;
  if (bulk.epoch!=v2Epoch) {
    portENTER_CRITICAL(&bulkTxMux);
    bulkTxEnabled=false; bulkTxAccepted=bulkTxCompleted=0;
    bulkTracker=BleBulkWire::TxTracker{};
    portEXIT_CRITICAL(&bulkTxMux);
  }
  if (v2BulkBusy && (bulk.epoch != v2Epoch || !bleDeviceConnected || !v2Subscribed || !bulkCccd->getNotifications()))
    bulkClose("cancelled", "LINK_RETIRED");
  if (!v2BulkBusy) return;
  bulkWait(BulkWait::None);
  portENTER_CRITICAL(&bulkTxMux);
  bool txFailed=bulkTxErrors!=0;
  bool txPending=bulkTxAccepted>bulkTxCompleted;
  portEXIT_CRITICAL(&bulkTxMux);
  if (txFailed) { bulkClose("failed","NATIVE_TX_ERROR"); return; }
  if (bulkBadAck) { bulkBadAck = false; bulkClose("failed", "BAD_ACK"); return; }
  BulkAckFrame ack;
  while (xQueueReceive(bulkAckQueue, &ack, 0) == pdTRUE) {
    if (ack.epoch != bulk.epoch || BleBulkWire::read32(ack.bytes + 4) != bulk.handle) continue;
    if (memcmp(ack.bytes, "ABK1", 4) || ack.bytes[14] || ack.bytes[15]) { bulkClose("failed", "BAD_ACK"); return; }
    uint16_t credits = ack.bytes[12] | (uint16_t(ack.bytes[13]) << 8);
    BleBulkWire::Ack outcome = bulk.window.ack(BleBulkWire::read32(ack.bytes + 8), credits);
    if (outcome == BleBulkWire::Ack::Invalid) { bulkClose("failed", "BAD_ACK_OFFSET"); return; }
    if (outcome == BleBulkWire::Ack::Ignored) continue;
    if (!bulk.started) { bulk.started = true; bulk.startedAt = millis(); }
    bulk.deadlineAt = millis(); bulk.failures = 0;
    if (outcome == BleBulkWire::Ack::Complete) { bulkClose("complete"); return; }
    bulk.state = "sending";
  }
  // Deadline bounds each window, including local resource waits; queries do not extend it.
  if (millis() - bulk.deadlineAt > 10000) { bulkClose("failed", "WINDOW_TIMEOUT"); return; }
  if (!bulk.window.canSend()) { bulkWait(BulkWait::Ack); return; }
  if (v2Tx.length()) return;
#if defined(CONFIG_NIMBLE_ENABLED)
  // One locally outstanding submission; receiver window remains 16 packets.
  if (txPending) { bulkWait(BulkWait::Tx); return; }
#endif
  uint32_t now = millis();
  if (now - bulk.lastSendAt < (bulk.failures ? 32U : 4U)) { bulkWait(BulkWait::Pacing); return; }
  if (!bulk.pendingBytes) {
    uint16_t liveLimit = BleBulkWire::valueBytes(bleServer->getPeerMTU(bleServer->getConnId()));
    if (liveLimit < bulk.valueLimit) { bulkClose("failed", "MTU_CHANGED"); return; }
    uint32_t remaining = bulk.meta.bytes - bulk.window.sent;
    uint16_t bytes = bulk.valueLimit - BleBulkWire::HeaderBytes;
    if (remaining < bytes) bytes = uint16_t(remaining);
    BleBulkWire::header(bulk.pending, bulk.handle, bulk.window.sent, bulk.meta.sensor, bytes, remaining == bytes);
    uint64_t readStart=esp_timer_get_time();
    bool readOk=bulk.file.seek(bulk.window.sent) && bulk.file.read(bulk.pending + BleBulkWire::HeaderBytes, bytes)==bytes;
    bulkTiming.sdReadUs+=esp_timer_get_time()-readStart;
    if (!readOk) {
      bulkClose("failed", "SD_READ_FAILED"); return;
    }
    bulk.pendingBytes = bytes + BleBulkWire::HeaderBytes;
  }
  bulkNotifyStatus = -1; bulkNotifyCode = 0;
  uint64_t submitStart=esp_timer_get_time();
#if defined(CONFIG_NIMBLE_ENABLED)
  // notify_custom consumes the mbuf regardless of its return code.
  os_mbuf* mbuf=ble_hs_mbuf_from_flat(bulk.pending,bulk.pendingBytes);
  portENTER_CRITICAL(&bulkTxMux);
  bulkTracker.begin();
  portEXIT_CRITICAL(&bulkTxMux);
  int submitCode=mbuf?ble_gatts_notify_custom(bleServer->getConnId(),bulkData->getHandle(),mbuf):BLE_HS_ENOMEM;
  portENTER_CRITICAL(&bulkTxMux);
  bulkTracker.result(submitCode);
  bulkTxAccepted=bulkTracker.accepted; bulkTxCompleted=bulkTracker.completed;
  bulkTxErrors=bulkTracker.errors; bulkTxLastError=bulkTracker.lastError;
  portEXIT_CRITICAL(&bulkTxMux);
  bulkNotifyStatus=submitCode==0?int(BLECharacteristicCallbacks::SUCCESS_NOTIFY):int(BLECharacteristicCallbacks::ERROR_GATT);
  bulkNotifyCode=submitCode;
#else
  bulkData->setValue(bulk.pending, bulk.pendingBytes);
  bulkData->notify();
#endif
  bulkTiming.notifySubmitUs+=esp_timer_get_time()-submitStart;
  ++bulkTiming.attempts;
  bulkTiming.lastSubmitCode=int(bulkNotifyCode);
  bulk.lastSendAt = millis();
  if (bulkNotifyStatus != int(BLECharacteristicCallbacks::SUCCESS_NOTIFY)) {
    bool noMemory = false;
    if (bulkNotifyStatus == int(BLECharacteristicCallbacks::ERROR_GATT)) {
#if defined(CONFIG_NIMBLE_ENABLED)
      noMemory = bulkNotifyCode == BLE_HS_ENOMEM;
#elif defined(CONFIG_BLUEDROID_ENABLED)
      noMemory = bulkNotifyCode == ESP_ERR_NO_MEM;
#endif
    }
    if (noMemory) ++bulkTiming.noMemory; else ++bulkTiming.otherRejects;
    if (noMemory && ++bulk.failures <= 8) { ++bulk.memoryRetries; return; }
    bulkClose("failed", "NOTIFICATION_SUBMIT"); return;
  }
#if !defined(CONFIG_NIMBLE_ENABLED)
  portENTER_CRITICAL(&bulkTxMux);
  ++bulkTxAccepted;
  portEXIT_CRITICAL(&bulkTxMux);
#endif
  if (!bulk.window.submitted(bulk.pendingBytes - BleBulkWire::HeaderBytes)) {
    bulkClose("failed", "INTERNAL_OFFSET"); return;
  }
  bulk.pendingBytes = 0; bulk.failures = 0; ++bulk.notifications;
  if (bulk.window.waiting()) { bulk.state = "awaiting_ack"; bulk.deadlineAt = millis(); }
}

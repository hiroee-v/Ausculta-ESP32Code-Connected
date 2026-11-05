#line 1 "C:\\Users\\Jerome Vargas\\Downloads\\Ausculta_Recorder_with_SD_Card_Module_All_8_Mics_WORKING__1_\\Ausculta_Recorder_with_SD_Card_Module_All_8_Mics_WORKING__1_.ino"
#include <Arduino.h>
#include <WiFi.h>
#include <WebServer.h>
#include <driver/i2s.h>
#include <SPI.h>
#include <SD.h>
#include "esp_timer.h"
#include <stdarg.h>

// Mirror BLE diagnostics to UART0 when Serial is routed to native USB CDC.
// The USB-to-UART port that prints ROM boot messages can then show these logs too.
#line 12 "C:\\Users\\Jerome Vargas\\Downloads\\Ausculta_Recorder_with_SD_Card_Module_All_8_Mics_WORKING__1_\\Ausculta_Recorder_with_SD_Card_Module_All_8_Mics_WORKING__1_.ino"
void bleLogf(const char* format, ...);
#line 94 "C:\\Users\\Jerome Vargas\\Downloads\\Ausculta_Recorder_with_SD_Card_Module_All_8_Mics_WORKING__1_\\Ausculta_Recorder_with_SD_Card_Module_All_8_Mics_WORKING__1_.ino"
void initBluetooth();
#line 264 "C:\\Users\\Jerome Vargas\\Downloads\\Ausculta_Recorder_with_SD_Card_Module_All_8_Mics_WORKING__1_\\Ausculta_Recorder_with_SD_Card_Module_All_8_Mics_WORKING__1_.ino"
void resetFilters();
#line 270 "C:\\Users\\Jerome Vargas\\Downloads\\Ausculta_Recorder_with_SD_Card_Module_All_8_Mics_WORKING__1_\\Ausculta_Recorder_with_SD_Card_Module_All_8_Mics_WORKING__1_.ino"
void selectMicrophone(uint8_t micIndex);
#line 285 "C:\\Users\\Jerome Vargas\\Downloads\\Ausculta_Recorder_with_SD_Card_Module_All_8_Mics_WORKING__1_\\Ausculta_Recorder_with_SD_Card_Module_All_8_Mics_WORKING__1_.ino"
void initI2S();
#line 313 "C:\\Users\\Jerome Vargas\\Downloads\\Ausculta_Recorder_with_SD_Card_Module_All_8_Mics_WORKING__1_\\Ausculta_Recorder_with_SD_Card_Module_All_8_Mics_WORKING__1_.ino"
void initSDCard();
#line 324 "C:\\Users\\Jerome Vargas\\Downloads\\Ausculta_Recorder_with_SD_Card_Module_All_8_Mics_WORKING__1_\\Ausculta_Recorder_with_SD_Card_Module_All_8_Mics_WORKING__1_.ino"
template <typename T>void writeWavHeader(T &stream, uint32_t dataSize);
#line 364 "C:\\Users\\Jerome Vargas\\Downloads\\Ausculta_Recorder_with_SD_Card_Module_All_8_Mics_WORKING__1_\\Ausculta_Recorder_with_SD_Card_Module_All_8_Mics_WORKING__1_.ino"
String sensorName(uint8_t sensor);
#line 373 "C:\\Users\\Jerome Vargas\\Downloads\\Ausculta_Recorder_with_SD_Card_Module_All_8_Mics_WORKING__1_\\Ausculta_Recorder_with_SD_Card_Module_All_8_Mics_WORKING__1_.ino"
String nextSensorFilename(uint8_t sensor);
#line 386 "C:\\Users\\Jerome Vargas\\Downloads\\Ausculta_Recorder_with_SD_Card_Module_All_8_Mics_WORKING__1_\\Ausculta_Recorder_with_SD_Card_Module_All_8_Mics_WORKING__1_.ino"
String createScanFolder();
#line 406 "C:\\Users\\Jerome Vargas\\Downloads\\Ausculta_Recorder_with_SD_Card_Module_All_8_Mics_WORKING__1_\\Ausculta_Recorder_with_SD_Card_Module_All_8_Mics_WORKING__1_.ino"
String nextScanFilename(const String &folder, uint8_t sensor);
#line 419 "C:\\Users\\Jerome Vargas\\Downloads\\Ausculta_Recorder_with_SD_Card_Module_All_8_Mics_WORKING__1_\\Ausculta_Recorder_with_SD_Card_Module_All_8_Mics_WORKING__1_.ino"
void saveWavToSD(const char* filename, int16_t* buffer, uint32_t count);
#line 467 "C:\\Users\\Jerome Vargas\\Downloads\\Ausculta_Recorder_with_SD_Card_Module_All_8_Mics_WORKING__1_\\Ausculta_Recorder_with_SD_Card_Module_All_8_Mics_WORKING__1_.ino"
int16_t processAudioSample(int32_t rawSample);
#line 1001 "C:\\Users\\Jerome Vargas\\Downloads\\Ausculta_Recorder_with_SD_Card_Module_All_8_Mics_WORKING__1_\\Ausculta_Recorder_with_SD_Card_Module_All_8_Mics_WORKING__1_.ino"
void recordingTask(void*);
#line 1064 "C:\\Users\\Jerome Vargas\\Downloads\\Ausculta_Recorder_with_SD_Card_Module_All_8_Mics_WORKING__1_\\Ausculta_Recorder_with_SD_Card_Module_All_8_Mics_WORKING__1_.ino"
void startRecording();
#line 1104 "C:\\Users\\Jerome Vargas\\Downloads\\Ausculta_Recorder_with_SD_Card_Module_All_8_Mics_WORKING__1_\\Ausculta_Recorder_with_SD_Card_Module_All_8_Mics_WORKING__1_.ino"
void stopRecording();
#line 1127 "C:\\Users\\Jerome Vargas\\Downloads\\Ausculta_Recorder_with_SD_Card_Module_All_8_Mics_WORKING__1_\\Ausculta_Recorder_with_SD_Card_Module_All_8_Mics_WORKING__1_.ino"
void recordAllMicsTask(void*);
#line 1263 "C:\\Users\\Jerome Vargas\\Downloads\\Ausculta_Recorder_with_SD_Card_Module_All_8_Mics_WORKING__1_\\Ausculta_Recorder_with_SD_Card_Module_All_8_Mics_WORKING__1_.ino"
bool bluetoothMtuReady();
#line 1267 "C:\\Users\\Jerome Vargas\\Downloads\\Ausculta_Recorder_with_SD_Card_Module_All_8_Mics_WORKING__1_\\Ausculta_Recorder_with_SD_Card_Module_All_8_Mics_WORKING__1_.ino"
void notifyBluetoothStatus();
#line 1359 "C:\\Users\\Jerome Vargas\\Downloads\\Ausculta_Recorder_with_SD_Card_Module_All_8_Mics_WORKING__1_\\Ausculta_Recorder_with_SD_Card_Module_All_8_Mics_WORKING__1_.ino"
void setup();
#line 1833 "C:\\Users\\Jerome Vargas\\Downloads\\Ausculta_Recorder_with_SD_Card_Module_All_8_Mics_WORKING__1_\\Ausculta_Recorder_with_SD_Card_Module_All_8_Mics_WORKING__1_.ino"
void loop();
#line 12 "C:\\Users\\Jerome Vargas\\Downloads\\Ausculta_Recorder_with_SD_Card_Module_All_8_Mics_WORKING__1_\\Ausculta_Recorder_with_SD_Card_Module_All_8_Mics_WORKING__1_.ino"
void bleLogf(const char* format, ...) {
  va_list args;
  va_start(args, format);
  Serial.vprintf(format, args);
  va_end(args);
#if ARDUINO_USB_CDC_ON_BOOT
  va_start(args, format);
  Serial0.vprintf(format, args);
  va_end(args);
#endif
}

// ADDED: Bluetooth BLE libraries
#include <BLEDevice.h>
#include <BLEServer.h>
#include <BLEUtils.h>
#include <BLE2902.h>

const char* AP_SSID = "AUSCULTA-RECORDER";
const char* AP_PASSWORD = "12345678";

WebServer server(80);

// AUSCULTA mobile GATT contract. Commands are raw UTF-8, not Base64.
#define BLE_DEVICE_NAME "AUSCULTA+ Smart Vest"
#define BLE_SERVICE_UUID "0000a001-0000-1000-8000-00805f9b34fb"
BLEServer* bleServer = nullptr;
BLECharacteristic* bleStatus = nullptr;
BLECharacteristic* bleAudio = nullptr;
QueueHandle_t bleCommands = nullptr;
volatile bool bleDeviceConnected = false;
volatile bool blePreviouslyConnected = false;
volatile bool bleDisconnected = false;
bool i2sReady = false;
volatile bool sdReady = false;
volatile bool v2OwnsRecorder = false;
volatile bool v2BulkBusy = false;
void initV2Gatt();
void retireV2Link();
void processV2Bluetooth();
void beginV2DiagnosticLink();
void recordV2Disconnect(int reason);
bool bleCapture = false;
volatile bool bleAcquisitionDone = false;
uint32_t bleReadSample = 0;
uint32_t bleSequence = 0;
void processBluetooth();

class AuscultaBLEServerCallbacks : public BLEServerCallbacks {
  void onConnect(BLEServer*) override {
    beginV2DiagnosticLink();
    bleDeviceConnected = true;
    bleLogf("[BLE] Device connected. freeHeap=%u stackFree=%u\n",
            ESP.getFreeHeap(), unsigned(uxTaskGetStackHighWaterMark(nullptr)));
  }
  void onDisconnect(BLEServer*) override {
    recordV2Disconnect(-1);
    retireV2Link();
    bleDeviceConnected = false;
    bleDisconnected = true;
    bleLogf("[BLE] Device disconnected.\n");
  }
#if defined(CONFIG_BLUEDROID_ENABLED)
  void onDisconnect(BLEServer* server, esp_ble_gatts_cb_param_t* param) override {
    recordV2Disconnect(int(param->disconnect.reason));
    bleLogf("[BLE] Native disconnect reason=0x%02x\n", unsigned(param->disconnect.reason));
    onDisconnect(server);
  }
#endif
};

class AuscultaCommandCallbacks : public BLECharacteristicCallbacks {
  void onWrite(BLECharacteristic* characteristic) override {
    auto value = characteristic->getValue();
    char command[32] = {};
    if (value.length() == 0 || value.length() >= sizeof(command)) return;
    memcpy(command, value.c_str(), value.length());
    if (xQueueSend(bleCommands, command, 0) != pdTRUE)
      Serial.println("[BLE] Command queue full; command not executed.");
  }
};

void initBluetooth() {
  bleCommands = xQueueCreate(8, 32);
  if (!bleCommands) { Serial.println("[BLE] Command queue allocation failed."); return; }
  BLEDevice::init(BLE_DEVICE_NAME);
  BLEDevice::setMTU(517);
  bleServer = BLEDevice::createServer();
  bleServer->setCallbacks(new AuscultaBLEServerCallbacks());
  // Five characteristics and four CCCDs need more than the default 15 handles.
  BLEService* service = bleServer->createService(BLEUUID(BLE_SERVICE_UUID), 32);
  BLECharacteristic* command = service->createCharacteristic(
    "0000a002-0000-1000-8000-00805f9b34fb", BLECharacteristic::PROPERTY_WRITE);
  command->setCallbacks(new AuscultaCommandCallbacks());
  const char* lanes[] = {
    "0000a003-0000-1000-8000-00805f9b34fb",
    "0000a004-0000-1000-8000-00805f9b34fb",
    "0000a005-0000-1000-8000-00805f9b34fb",
    "0000a006-0000-1000-8000-00805f9b34fb"
  };
  for (int i = 0; i < 4; ++i) {
    BLECharacteristic* lane = service->createCharacteristic(lanes[i], BLECharacteristic::PROPERTY_NOTIFY);
    lane->addDescriptor(new BLE2902());
    if (i == 0) bleStatus = lane;
    if (i == 3) bleAudio = lane;
  }
  service->start();
  initV2Gatt();
  BLEAdvertising* advertising = BLEDevice::getAdvertising();
  advertising->addServiceUUID(BLE_SERVICE_UUID);
  advertising->setScanResponse(true);
  BLEDevice::startAdvertising();
  Serial.println("[BLE] Advertising " BLE_SERVICE_UUID);
}

// --- EXACT PIN MAPPING MATCHING YOUR WIRING ---
#define I2S_PORT            I2S_NUM_0
#define I2S_BCLK_PIN        41
#define I2S_WS_PIN          42
#define I2S_SD_PIN          17

// 74HC4051 Multiplexer Control Pins
#define MUX_S0              5
#define MUX_S1              6
#define MUX_S2              7

// MicroSD Card Pin Assignments
#define SD_CS_PIN           15
#define SPI_SCK_PIN         2
#define SPI_MOSI_PIN        4
#define SPI_MISO_PIN        16

#define SAMPLE_RATE         16000
#define RECORD_SECONDS      20
#define MAX_SAMPLES         (SAMPLE_RATE * RECORD_SECONDS)

int16_t* audioBuffer = NULL;

volatile bool recording = false;
volatile bool scanningAll = false;
volatile bool recordingFinished = false;
volatile uint32_t sampleCount = 0;
volatile bool recordingTaskRunning = false;

volatile bool stopRequested = false;
volatile uint8_t currentScanSensor = 0;

// ADDED: Auto-scan timer states
volatile bool scanResting = false;
volatile uint32_t scanPhaseStart = 0;
volatile uint32_t scanPhaseDuration = 0;

TaskHandle_t recordingTaskHandle = NULL;
TaskHandle_t scanTaskHandle = NULL;

uint8_t selectedMic = 1;
String statusMessage = "Ready";
String currentScanFolder = "";

#define FILTER_LOW_HZ          80.0f
#define FILTER_HIGH_HZ       1200.0f
#define FILTER_NOTCH_HZ        60.0f
#define SIGNAL_GAIN             4.0f
#define TRANSIENT_SPIKE_LIMIT 30000.0f

volatile int16_t recordedMin = 0;
volatile int16_t recordedMax = 0;
volatile uint32_t rmsLevel = 0;

uint32_t fileCounter = 0;
uint32_t scanCounter = 1;

struct Biquad {
  float b0, b1, b2, a1, a2, x1, x2, y1, y2;
};

void setupLowPass(Biquad &f, float frequency, float Q);
void setupHighPass(Biquad &f, float frequency, float Q);
void setupNotch(Biquad &f, float frequency, float Q);
float processBiquad(Biquad &f, float input);

Biquad highPass, notch60, lowPass;

void setupLowPass(Biquad &f, float frequency, float Q) {
  float w0 = 2.0f * PI * frequency / SAMPLE_RATE;
  float cosw0 = cosf(w0), sinw0 = sinf(w0);
  float alpha = sinw0 / (2.0f * Q);
  float b0 = (1.0f - cosw0) / 2.0f;
  float b1 = 1.0f - cosw0;
  float b2 = (1.0f - cosw0) / 2.0f;
  float a0 = 1.0f + alpha;
  float a1 = -2.0f * cosw0;
  float a2 = 1.0f - alpha;

  f.b0 = b0 / a0;
  f.b1 = b1 / a0;
  f.b2 = b2 / a0;
  f.a1 = a1 / a0;
  f.a2 = a2 / a0;
  f.x1 = f.x2 = f.y1 = f.y2 = 0;
}

void setupHighPass(Biquad &f, float frequency, float Q) {
  float w0 = 2.0f * PI * frequency / SAMPLE_RATE;
  float cosw0 = cosf(w0), sinw0 = sinf(w0);
  float alpha = sinw0 / (2.0f * Q);
  float b0 = (1.0f + cosw0) / 2.0f;
  float b1 = -(1.0f + cosw0);
  float b2 = (1.0f + cosw0) / 2.0f;
  float a0 = 1.0f + alpha;
  float a1 = -2.0f * cosw0;
  float a2 = 1.0f - alpha;

  f.b0 = b0 / a0;
  f.b1 = b1 / a0;
  f.b2 = b2 / a0;
  f.a1 = a1 / a0;
  f.a2 = a2 / a0;
  f.x1 = f.x2 = f.y1 = f.y2 = 0;
}

void setupNotch(Biquad &f, float frequency, float Q) {
  float w0 = 2.0f * PI * frequency / SAMPLE_RATE;
  float cosw0 = cosf(w0), sinw0 = sinf(w0);
  float alpha = sinw0 / (2.0f * Q);
  float b0 = 1.0f;
  float b1 = -2.0f * cosw0;
  float b2 = 1.0f;
  float a0 = 1.0f + alpha;
  float a1 = -2.0f * cosw0;
  float a2 = 1.0f - alpha;

  f.b0 = b0 / a0;
  f.b1 = b1 / a0;
  f.b2 = b2 / a0;
  f.a1 = a1 / a0;
  f.a2 = a2 / a0;
  f.x1 = f.x2 = f.y1 = f.y2 = 0;
}

float processBiquad(Biquad &f, float input) {
  float output = f.b0 * input + f.b1 * f.x1 + f.b2 * f.x2
               - f.a1 * f.y1 - f.a2 * f.y2;

  f.x2 = f.x1;
  f.x1 = input;
  f.y2 = f.y1;
  f.y1 = output;

  return output;
}

void resetFilters() {
  highPass.x1 = highPass.x2 = highPass.y1 = highPass.y2 = 0;
  notch60.x1 = notch60.x2 = notch60.y1 = notch60.y2 = 0;
  lowPass.x1 = lowPass.x2 = lowPass.y1 = lowPass.y2 = 0;
}

void selectMicrophone(uint8_t micIndex) {
  if (micIndex < 1) micIndex = 1;
  if (micIndex > 8) micIndex = 8;

  selectedMic = micIndex;

  uint8_t channel = micIndex - 1;

  digitalWrite(MUX_S0, (channel & 0x01) ? HIGH : LOW);
  digitalWrite(MUX_S1, (channel & 0x02) ? HIGH : LOW);
  digitalWrite(MUX_S2, (channel & 0x04) ? HIGH : LOW);

  delay(15);
}

void initI2S() {
  i2s_config_t i2s_config = {
    .mode = (i2s_mode_t)(I2S_MODE_MASTER | I2S_MODE_RX),
    .sample_rate = SAMPLE_RATE,
    .bits_per_sample = I2S_BITS_PER_SAMPLE_32BIT,
    .channel_format = I2S_CHANNEL_FMT_ONLY_LEFT,
    .communication_format = I2S_COMM_FORMAT_STAND_I2S,
    .intr_alloc_flags = ESP_INTR_FLAG_LEVEL1,
    .dma_buf_count = 8,
    .dma_buf_len = 256,
    .use_apll = false,
    .tx_desc_auto_clear = false,
    .fixed_mclk = 0
  };

  i2s_pin_config_t pin_config = {
    .bck_io_num = I2S_BCLK_PIN,
    .ws_io_num = I2S_WS_PIN,
    .data_out_num = I2S_PIN_NO_CHANGE,
    .data_in_num = I2S_SD_PIN
  };

  i2sReady = i2s_driver_install(I2S_PORT, &i2s_config, 0, NULL) == ESP_OK;
  if (i2sReady) i2sReady = i2s_set_pin(I2S_PORT, &pin_config) == ESP_OK;
  if (i2sReady) i2sReady = i2s_start(I2S_PORT) == ESP_OK;
  Serial.printf("[I2S] Driver ready: %s (does not verify microphone wiring)\n", i2sReady ? "yes" : "no");
}

void initSDCard() {
  SPI.begin(SPI_SCK_PIN, SPI_MISO_PIN, SPI_MOSI_PIN, SD_CS_PIN);

  if (!SD.begin(SD_CS_PIN)) {
    Serial.println("[SD] Initialization failed! Check card wiring or FAT32 format.");
    return;
  }

  sdReady = true;
  Serial.println("[SD] MicroSD Card initialized successfully.");
}

template <typename T>
void writeWavHeader(T &stream, uint32_t dataSize) {
  uint32_t fileSize = 36 + dataSize;
  uint32_t fmtSize = 16;

  uint16_t audioFormat = 1;
  uint16_t channels = 1;
  uint16_t bitsPerSample = 16;
  uint16_t blockAlign = channels * bitsPerSample / 8;

  uint32_t sampleRate = SAMPLE_RATE;
  uint32_t byteRate = sampleRate * blockAlign;

  stream.write((const uint8_t*)"RIFF", 4);
  stream.write((uint8_t*)&fileSize, 4);
  stream.write((const uint8_t*)"WAVE", 4);
  stream.write((const uint8_t*)"fmt ", 4);
  stream.write((uint8_t*)&fmtSize, 4);
  stream.write((uint8_t*)&audioFormat, 2);
  stream.write((uint8_t*)&channels, 2);
  stream.write((uint8_t*)&sampleRate, 4);
  stream.write((uint8_t*)&byteRate, 4);
  stream.write((uint8_t*)&blockAlign, 2);
  stream.write((uint8_t*)&bitsPerSample, 2);
  stream.write((const uint8_t*)"data", 4);
  stream.write((uint8_t*)&dataSize, 4);
}

const char* SENSOR_NAMES[] = {
  "Sensor 1 (Upper Right Chest)",
  "Sensor 2 (Upper Left Chest)",
  "Sensor 3 (Lower Right Chest)",
  "Sensor 4 (Lower Left Chest)",
  "Sensor 5 (Upper Right Back)",
  "Sensor 6 (Upper Left Back)",
  "Sensor 7 (Lower Right Back)",
  "Sensor 8 (Lower Left Back)"
};

String sensorName(uint8_t sensor) {
  if (sensor < 1 || sensor > 8) return "Unknown Sensor";
  return SENSOR_NAMES[sensor - 1];
}

// --------------------------------------------------
// FILE AND FOLDER MANAGEMENT
// --------------------------------------------------

String nextSensorFilename(uint8_t sensor) {
  const String base = "/" + sensorName(sensor);

  String filename = base + ".wav";
  uint32_t number = 2;

  while (SD.exists(filename.c_str())) {
    filename = base + " (" + String(number++) + ").wav";
  }

  return filename;
}

String createScanFolder() {
  while (true) {
    char folderName[20];
    snprintf(folderName, sizeof(folderName), "/Scan_%03lu",
             (unsigned long)scanCounter);

    scanCounter++;

    if (!SD.exists(folderName)) {
      if (SD.mkdir(folderName)) {
        Serial.printf("[SCAN] Created folder: %s\n", folderName);
        return String(folderName);
      }

      Serial.printf("[SCAN] Failed to create folder: %s\n", folderName);
      return "";
    }
  }
}

String nextScanFilename(const String &folder, uint8_t sensor) {
  String base = folder + "/" + sensorName(sensor);

  String filename = base + ".wav";
  uint32_t number = 2;

  while (SD.exists(filename.c_str())) {
    filename = base + " (" + String(number++) + ").wav";
  }

  return filename;
}

void saveWavToSD(const char* filename, int16_t* buffer, uint32_t count) {
  if (count == 0) {
    Serial.println("[SD] No audio samples to save.");
    return;
  }

  if (SD.exists(filename)) {
    Serial.printf("[SD] Refusing to overwrite existing recording: %s\n", filename);
    return;
  }

  File file = SD.open(filename, FILE_WRITE);

  if (!file) {
    Serial.println("[SD] Failed to open file for writing.");
    return;
  }

  uint32_t dataSize = count * sizeof(int16_t);

  writeWavHeader(file, dataSize);

  const size_t chunkSize = 512;
  uint8_t* bytePtr = (uint8_t*)buffer;
  uint32_t remainingBytes = dataSize;

  while (remainingBytes > 0) {
    size_t toWrite = (remainingBytes < chunkSize)
                     ? remainingBytes : chunkSize;

    file.write(bytePtr, toWrite);

    bytePtr += toWrite;
    remainingBytes -= toWrite;
  }

  file.close();

  Serial.printf("[SD] Saved recording to %s (%u bytes)\n",
                filename, dataSize + 44);
}



// --------------------------------------------------
// AUDIO PROCESSING
// --------------------------------------------------

int16_t processAudioSample(int32_t rawSample) {
  int16_t rawPCM = (int16_t)(rawSample >> 16);

  float signal = static_cast<float>(rawPCM);

  signal = processBiquad(highPass, signal);
  signal = processBiquad(notch60, signal);
  signal = processBiquad(lowPass, signal);

  signal *= SIGNAL_GAIN;

  if (signal > TRANSIENT_SPIKE_LIMIT) signal = TRANSIENT_SPIKE_LIMIT;
  if (signal < -TRANSIENT_SPIKE_LIMIT) signal = -TRANSIENT_SPIKE_LIMIT;

  if (signal > 32767.0f) signal = 32767.0f;
  if (signal < -32768.0f) signal = -32768.0f;

  return static_cast<int16_t>(signal);
}

// --------------------------------------------------
// WEB PAGE
// --------------------------------------------------

const char MAIN_PAGE[] PROGMEM = R"rawliteral(
<!DOCTYPE html>
<html>
<head>
<meta name="viewport" content="width=device-width, initial-scale=1">
<title>AUSCULTA Electronic Stethoscope</title>

<style>
body{
  font-family:Arial,sans-serif;
  background:#101820;
  color:white;
  margin:0
}

.container{
  max-width:500px;
  margin:auto;
  padding:30px 20px
}

button,select{
  width:100%;
  padding:14px;
  margin:6px 0;
  border:none;
  border-radius:12px;
  font-size:16px;
  font-weight:bold
}

select{
  background:#222;
  color:white
}

.record{
  background:#e53935;
  color:white
}

.scan{
  background:#7b1fa2;
  color:white
}

.stop{
  background:#555;
  color:white
}

.play{
  background:#2196f3;
  color:white
}

button:disabled{
  opacity:0.45
}

audio{
  width:100%;
  margin-top:15px
}

.section{
  margin-top:25px;
  padding-top:15px;
  border-top:1px solid #39434d
}

.status{
  margin-top:15px;
  font-weight:bold
}

.timer{
  font-size:22px;
  margin:12px 0
}

.subtitle{
  color:#aaa
}
</style>
</head>

<body>
<div class="container">

<h1>AUSCULTA Stethoscope</h1>
<div class="subtitle">ESP32-S3 + Mux + SD Card</div>

<div class="status" id="status">Ready</div>

<!-- ADDED: Bluetooth connection status -->
<div class="status" id="bleStatus">
  Bluetooth: Checking...
</div>

<div class="timer" id="timer">0.0s</div>

<label>Select Sensor Zone (Manual):</label>

<select id="micSelect">
  <option value="1">Sensor 1 (Upper Right Chest)</option>
  <option value="2">Sensor 2 (Upper Left Chest)</option>
  <option value="3">Sensor 3 (Lower Right Chest)</option>
  <option value="4">Sensor 4 (Lower Left Chest)</option>
  <option value="5">Sensor 5 (Upper Right Back)</option>
  <option value="6">Sensor 6 (Upper Left Back)</option>
  <option value="7">Sensor 7 (Lower Right Back)</option>
  <option value="8">Sensor 8 (Lower Left Back)</option>
</select>

<button onclick="setMic()">Set Channel</button>

<button class="record" id="recordBtn" onclick="startRecording()">
RECORD SINGLE (20s)
</button>

<button class="scan" id="scanBtn" onclick="recordAllMics()">
AUTO-SCAN ALL 8 ZONES (20s + 5s Break)
</button>

<button class="stop" id="stopBtn" onclick="stopRecording()" disabled>
STOP
</button>

<div class="section">
<h3>Single Recordings</h3>

<label>Select Recorded File:</label>
<select id="fileSelect"></select>

<button class="play" id="playBtn" onclick="playRecording('single')" disabled>
PLAY SELECTED
</button>

<button class="play" id="downloadBtn" onclick="downloadRecording('single')" disabled>
DOWNLOAD SELECTED (.wav)
</button>
</div>

<div class="section">
<h3>Scan Recordings</h3>

<label>Select Scan Folder:</label>
<select id="scanSelect" onchange="loadScanFiles()"></select>

<label>Select Sensor Recording:</label>
<select id="scanFileSelect"></select>

<button class="play" id="scanPlayBtn" onclick="playRecording('scan')" disabled>
PLAY SCAN RECORDING
</button>

<button class="play" id="scanDownloadBtn" onclick="downloadRecording('scan')" disabled>
DOWNLOAD SCAN RECORDING (.wav)
</button>
</div>

<audio id="audioPlayer" controls></audio>

</div>

<script>
let timerInterval;
let startTime;
let statusInterval;
let activeMode = "idle";

function setButtonsBusy(busy){
  document.getElementById("recordBtn").disabled = busy;
  document.getElementById("scanBtn").disabled = busy;
  document.getElementById("stopBtn").disabled = !busy;
}

// ADDED: Update Bluetooth connection status
async function updateBluetoothStatus(){

  try{

    let response = await fetch("/ble-status?t=" + Date.now());

    if(!response.ok) return;

    let data = await response.json();

    document.getElementById("bleStatus").innerText =
      data.connected
        ? "Bluetooth: Connected"
        : "Bluetooth: Disconnected";

  }catch(error){

    document.getElementById("bleStatus").innerText =
      "Bluetooth: Unavailable";
  }
}

setInterval(updateBluetoothStatus, 1000);
updateBluetoothStatus();

async function loadFileList(){
  try{
    let res = await fetch("/list?t=" + Date.now());
    if(!res.ok) throw new Error("File list request failed");

    let files = await res.json();

    let select = document.getElementById("fileSelect");
    select.innerHTML = "";

    files.forEach(f => {
      let opt = document.createElement("option");
      opt.value = f;
      opt.innerText = f.substring(1);
      select.appendChild(opt);
    });

    document.getElementById("playBtn").disabled = files.length === 0;
    document.getElementById("downloadBtn").disabled = files.length === 0;

  }catch(e){
    console.error("Failed to load single recordings");
  }
}

async function loadScanFolders(){
  try{
    let res = await fetch("/scans?t=" + Date.now());
    if(!res.ok) throw new Error("Scan folder request failed");

    let folders = await res.json();

    let select = document.getElementById("scanSelect");
    select.innerHTML = "";

    folders.forEach(folder => {
      let opt = document.createElement("option");
      opt.value = folder;
      opt.innerText = folder.substring(1);
      select.appendChild(opt);
    });

    if(folders.length > 0){
      select.selectedIndex = folders.length - 1;
      await loadScanFiles();
    }else{
      document.getElementById("scanFileSelect").innerHTML = "";
      document.getElementById("scanPlayBtn").disabled = true;
      document.getElementById("scanDownloadBtn").disabled = true;
    }

  }catch(e){
    console.error("Failed to load scan folders");
  }
}

async function loadScanFiles(){
  try{
    let folder = document.getElementById("scanSelect").value;

    if(!folder){
      document.getElementById("scanFileSelect").innerHTML = "";
      return;
    }

    let res = await fetch("/scanfiles?folder=" +
                          encodeURIComponent(folder) + "&t=" + Date.now());

    if(!res.ok) throw new Error("Scan files request failed");

    let files = await res.json();

    let select = document.getElementById("scanFileSelect");
    select.innerHTML = "";

    files.forEach(f => {
      let opt = document.createElement("option");
      opt.value = f;
      opt.innerText = f.substring(f.lastIndexOf("/") + 1);
      select.appendChild(opt);
    });

    document.getElementById("scanPlayBtn").disabled = files.length === 0;
    document.getElementById("scanDownloadBtn").disabled = files.length === 0;

  }catch(e){
    console.error("Failed to load scan recordings");
  }
}

async function setMic(){
  let mic = document.getElementById("micSelect").value;

  try{
    let res = await fetch("/mic?channel=" + mic);
    let data = await res.text();
    alert(data);
  }catch(e){
    alert("Connection error");
  }
}

async function startRecording(){
  if(activeMode !== "idle") return;

  activeMode = "single";
  setButtonsBusy(true);

  document.getElementById("playBtn").disabled = true;
  document.getElementById("downloadBtn").disabled = true;

  document.getElementById("status").innerText = "Starting single recording...";
  document.getElementById("timer").innerText = "0.0s";

  try{
    let mic = document.getElementById("micSelect").value;

    let response = await fetch("/record/start?channel=" + mic);

    if(!response.ok){
      throw new Error("Start failed");
    }

    startTime = Date.now();
    timerInterval = setInterval(updateTimer, 100);
    startStatusPolling();

  }catch(error){
    activeMode = "idle";
    setButtonsBusy(false);
    document.getElementById("status").innerText = "Connection error";
  }
}

async function recordAllMics(){
  if(activeMode !== "idle") return;

  activeMode = "scan";
  setButtonsBusy(true);

  document.getElementById("scanPlayBtn").disabled = true;
  document.getElementById("scanDownloadBtn").disabled = true;

  document.getElementById("status").innerText = "Starting scan...";
  document.getElementById("timer").innerText = "Preparing scan";

  try{
    let response = await fetch("/record/all");
    let data = await response.text();

    if(!response.ok){
      throw new Error(data);
    }

    document.getElementById("status").innerText = data;
    startStatusPolling();

  }catch(error){
    activeMode = "idle";
    setButtonsBusy(false);
    document.getElementById("status").innerText = "Scan failed to start";
  }
}

function updateTimer(){
  if(activeMode !== "single") return;

  let elapsed = (Date.now() - startTime) / 1000;

  if(elapsed > 20) elapsed = 20;

  document.getElementById("timer").innerText = elapsed.toFixed(1) + "s";

  if(elapsed >= 20){
    clearInterval(timerInterval);
  }
}

function startStatusPolling(){
  clearInterval(statusInterval);

  statusInterval = setInterval(async function(){
    try{
      let res = await fetch("/status?t=" + Date.now());
      if(!res.ok) return;

      let data = await res.json();

      document.getElementById("status").innerText = data.message;

      // UPDATED: Auto-scan recording and rest countdown
      if(data.mode === "scan"){

        let timer = document.getElementById("timer");

        if(data.resting){

          let remaining = data.remaining / 1000;

          timer.innerText = "Rest: " + remaining.toFixed(1) + "s";

        }else if(data.sensor > 0){

          let elapsed = data.elapsed / 1000;

          timer.innerText = elapsed.toFixed(1) + "s / 20.0s";

        }else{

          timer.innerText = "Preparing scan...";
        }
      }

      if(data.mode === "idle"){
        clearInterval(statusInterval);
        clearInterval(timerInterval);

        activeMode = "idle";
        setButtonsBusy(false);

        document.getElementById("timer").innerText = "Ready";

        await loadFileList();
        await loadScanFolders();
      }

    }catch(e){
      console.error("Status update failed");
    }
  }, 500);
}

async function stopRecording(){
  if(activeMode === "idle") return;

  document.getElementById("stopBtn").disabled = true;
  document.getElementById("status").innerText = "Stopping...";

  clearInterval(timerInterval);

  try{
    let response = await fetch("/record/stop");

    if(!response.ok){
      throw new Error("Stop request failed");
    }

    document.getElementById("status").innerText =
      "Stop requested. Saving current recording...";

  }catch(error){
    document.getElementById("status").innerText = "Connection error";
    document.getElementById("stopBtn").disabled = false;
  }
}

function getSelectedRecording(type){
  if(type === "single"){
    return document.getElementById("fileSelect").value;
  }

  return document.getElementById("scanFileSelect").value;
}

function playRecording(type){
  let filename = getSelectedRecording(type);
  if(!filename) return;

  let audio = document.getElementById("audioPlayer");

  audio.src = "/audio?file=" + encodeURIComponent(filename) +
              "&t=" + Date.now();

  audio.load();
  audio.play();

  document.getElementById("status").innerText =
    "Playing " + filename.substring(filename.lastIndexOf("/") + 1);
}

function downloadRecording(type){
  let filename = getSelectedRecording(type);
  if(!filename) return;

  const link = document.createElement("a");

  link.href = "/audio?file=" + encodeURIComponent(filename) + "&download=1";
  link.download = filename.substring(filename.lastIndexOf("/") + 1);

  document.body.appendChild(link);
  link.click();
  link.remove();
}

loadFileList();
loadScanFolders();
</script>

</body>
</html>
)rawliteral";

// --------------------------------------------------
// SINGLE RECORDING TASK
// --------------------------------------------------

void recordingTask(void*) {
  recordingTaskRunning = true;

  int16_t localMin = 32767;
  int16_t localMax = -32768;

  uint64_t sumSquares = 0;

  int32_t dmaBuffer[128];
  size_t bytesRead = 0;

  while (recording && sampleCount < MAX_SAMPLES) {

    if (i2s_read(I2S_PORT, dmaBuffer, sizeof(dmaBuffer),
                 &bytesRead, pdMS_TO_TICKS(100)) == ESP_OK && bytesRead > 0) {

      size_t samplesRead = bytesRead / sizeof(int32_t);

      for (size_t i = 0; i < samplesRead && sampleCount < MAX_SAMPLES; i++) {

        int16_t pcm = processAudioSample(dmaBuffer[i]);

        if (pcm < localMin) localMin = pcm;
        if (pcm > localMax) localMax = pcm;

        sumSquares += static_cast<int32_t>(pcm) *
                      static_cast<int32_t>(pcm);

        audioBuffer[sampleCount] = pcm;
        __sync_synchronize();
        sampleCount++;
      }
    }

    vTaskDelay(1);
  }

  recordedMin = localMin;
  recording = false;
  __sync_synchronize();
  bleAcquisitionDone = true;
  recordedMax = localMax;

  rmsLevel = sampleCount > 0
    ? static_cast<uint32_t>(
        sqrt(static_cast<double>(sumSquares) / sampleCount))
    : 0;

  if (sampleCount > 0) {
    fileCounter++;

    const String filename = nextSensorFilename(selectedMic);
    saveWavToSD(filename.c_str(), audioBuffer, sampleCount);
  }

  recording = false;
  recordingFinished = true;
  recordingTaskRunning = false;
  recordingTaskHandle = NULL;

  vTaskDelete(NULL);
}

void startRecording() {
  if (v2OwnsRecorder || v2BulkBusy) return;
  if (bleCapture || recording || recordingTaskRunning || scanningAll) return;

  sampleCount = 0;
  recordingFinished = false;

  recordedMin = 0;
  recordedMax = 0;
  rmsLevel = 0;

  stopRequested = false;

  resetFilters();

  recording = true;
  recordingTaskRunning = true;

  BaseType_t result = xTaskCreatePinnedToCore(
    recordingTask,
    "RecordingTask",
    4096,
    NULL,
    3,
    &recordingTaskHandle,
    0
  );

  if (result != pdPASS) {
    recording = false;
    recordingTaskRunning = false;
    recordingTaskHandle = NULL;
    Serial.println("[RECORD] Failed to create recording task.");
  }
}

// --------------------------------------------------
// STOP FUNCTION
// --------------------------------------------------

void stopRecording() {
  stopRequested = true;

  if (scanningAll) {
    Serial.println("[SCAN] Stop requested.");
    return;
  }

  if (recording) {
    recording = false;

    unsigned long timeout = millis() + 2000;

    while (recordingTaskRunning && millis() < timeout) {
      delay(1);
    }
  }
}

// --------------------------------------------------
// AUTO-SCAN TASK
// --------------------------------------------------

void recordAllMicsTask(void*) {

  Serial.println("[SCAN] Starting 8-sensor scan.");

  currentScanSensor = 0;

  String scanFolder = currentScanFolder;

  for (uint8_t ch = 1; ch <= 8; ch++) {

    if (stopRequested) {
      Serial.println("[SCAN] Scan stopped before next sensor.");
      break;
    }

    currentScanSensor = ch;

    Serial.printf("[SCAN] Recording Sensor %u: %s\n",
                  ch, sensorName(ch).c_str());

    selectMicrophone(ch);

    i2s_zero_dma_buffer(I2S_PORT);
    resetFilters();

    sampleCount = 0;

    int16_t localMin = 32767;
    int16_t localMax = -32768;

    uint64_t sumSquares = 0;

    int32_t dmaBuffer[128];
    size_t bytesRead = 0;

    // ADDED: Start the 20-second recording timer
    scanResting = false;
    scanPhaseStart = millis();
    scanPhaseDuration = 20000;

    unsigned long startMicTime = millis();

    // Record this sensor for up to 20 seconds.
    // The Stop button can interrupt this loop.
    while ((millis() - startMicTime < 20000) &&
           sampleCount < MAX_SAMPLES &&
           !stopRequested) {

      if (i2s_read(I2S_PORT, dmaBuffer, sizeof(dmaBuffer),
                   &bytesRead, pdMS_TO_TICKS(100)) == ESP_OK && bytesRead > 0) {

        size_t samplesRead = bytesRead / sizeof(int32_t);

        for (size_t i = 0; i < samplesRead &&
                           sampleCount < MAX_SAMPLES &&
                           !stopRequested; i++) {

          int16_t pcm = processAudioSample(dmaBuffer[i]);

          if (pcm < localMin) localMin = pcm;
          if (pcm > localMax) localMax = pcm;

          sumSquares += static_cast<int32_t>(pcm) *
                        static_cast<int32_t>(pcm);

          audioBuffer[sampleCount] = pcm;
        __sync_synchronize();
        sampleCount++;
        }
      }

      vTaskDelay(1);
    }

    // Save the current sensor recording, including partial audio
    // if the user pressed Stop during this sensor.
    if (sampleCount > 0) {

      String filename = nextScanFilename(scanFolder, ch);

      saveWavToSD(filename.c_str(), audioBuffer, sampleCount);

      Serial.printf("[SCAN] Saved Sensor %u (%u samples).\n",
                    ch, (unsigned int)sampleCount);
    } else {
      Serial.printf("[SCAN] Sensor %u has no samples to save.\n", ch);
    }

    if (stopRequested) {
      Serial.println("[SCAN] Stopping scan after current sensor.");
      break;
    }

    // Rest for 5 seconds before proceeding to the next sensor.
    if (ch < 8) {

      Serial.printf("[SCAN] Resting 5 seconds before Sensor %u...\n",
                    ch + 1);

      // ADDED: Start the 5-second rest countdown
      scanResting = true;
      scanPhaseStart = millis();
      scanPhaseDuration = 5000;

      unsigned long pauseStart = millis();

      while (millis() - pauseStart < 5000 && !stopRequested) {
        vTaskDelay(pdMS_TO_TICKS(20));
      }

      scanResting = false;

      if (stopRequested) {
        Serial.println("[SCAN] Stop requested during rest period.");
        break;
      }
    }
  }

  // ADDED: Reset timer states when scan finishes
  scanResting = false;
  scanPhaseStart = 0;
  scanPhaseDuration = 0;

  currentScanSensor = 0;
  scanningAll = false;
  stopRequested = false;

  Serial.println("[SCAN] Scan process finished.");

  scanTaskHandle = NULL;

  vTaskDelete(NULL);
}

// Keep BLE callbacks short: execute commands and send packets in loop().
bool bluetoothMtuReady() {
  return bleDeviceConnected && bleServer->getPeerMTU(bleServer->getConnId()) >= 179;
}

void notifyBluetoothStatus() {
  if (!bluetoothMtuReady()) return;
  char json[110];
  snprintf(json, sizeof(json),
    "{\"type\":\"status\",\"connected\":true,\"recording\":%s,\"sensorReady\":%s}",
    (recording || scanningAll || bleCapture) ? "true" : "false",
    (i2sReady && audioBuffer) ? "true" : "false");
  bleStatus->setValue((uint8_t*)json, strlen(json));
  bleStatus->notify();
}

void processBluetooth() {
  if (!bleCommands) return;
  if (bleDisconnected) {
    bleDisconnected = false;
    if (bleCapture) recording = false;
    bleCapture = false;
    xQueueReset(bleCommands);
    bleServer->startAdvertising();
    Serial.println("[BLE] Advertising restarted.");
  }
  char command[32];
  while (xQueueReceive(bleCommands, command, 0) == pdTRUE) {
    if (v2OwnsRecorder || v2BulkBusy) {
      // v2 owns acquisition; v1 commands cannot change its state or send audio.
      if (!strcmp(command, "PING") || !strcmp(command, "GET_STATUS")) notifyBluetoothStatus();
      continue;
    }
    Serial.printf("[BLE] Command: %s | MTU: %u\n", command,
                  bleServer->getPeerMTU(bleServer->getConnId()));
    if (!bluetoothMtuReady()) {
      Serial.println("[BLE] MTU below 179; reconnect before recording.");
      continue;
    }
    if (!strcmp(command, "START_RECORDING")) {
      if (!recording && !recordingTaskRunning && !scanningAll && !bleCapture && i2sReady && audioBuffer) {
        bleReadSample = 0;
        bleSequence = 0;
        bleAcquisitionDone = false;
        i2s_zero_dma_buffer(I2S_PORT);
        startRecording();
        bleCapture = recordingTaskRunning;
      }
      notifyBluetoothStatus();
    } else if (!strcmp(command, "STOP_RECORDING")) {
      // Confirm stopped after pending PCM and its final marker are sent.
      if (bleCapture) recording = false;
      else { stopRecording(); notifyBluetoothStatus(); }
    } else if (!strcmp(command, "PING") || !strcmp(command, "GET_STATUS")) {
      notifyBluetoothStatus(); // PONG is not accepted by the app parser.
    } else if (!strcmp(command, "GET_BATTERY")) {
      Serial.println("[BLE] Battery measurement unavailable; no fabricated value sent.");
    } else {
      Serial.println("[BLE] Unsupported command.");
    }
  }
  if (!bleCapture || !bluetoothMtuReady()) return;
  // Retain a frame until capture completes so the final real samples can carry STOP.
  bool finished = bleAcquisitionDone;
  uint32_t available = sampleCount - bleReadSample;
  for (int budget = 0; budget < 8 && (available > 80 || (finished && available > 0)); ++budget) {
    uint16_t count = available < 80 ? available : 80;
    uint16_t flags = 0x0020 | ((finished && available <= 80) ? 0x0008 : 0);
    uint8_t frame[176] = {'A', 'U', 'S', 'C', 1, 1};
    frame[6] = SAMPLE_RATE & 0xff; frame[7] = SAMPLE_RATE >> 8;
    for (int i = 0; i < 4; ++i) frame[8+i] = (bleSequence >> (8*i)) & 0xff;
    frame[12] = count & 0xff; frame[13] = count >> 8;
    frame[14] = flags & 0xff; frame[15] = flags >> 8;
    for (uint16_t i = 0; i < count; ++i) {
      uint16_t pcm = (uint16_t)audioBuffer[bleReadSample+i];
      frame[16+2*i] = pcm & 0xff; frame[17+2*i] = pcm >> 8;
    }
    bleAudio->setValue(frame, 16 + count*2);
    bleAudio->notify();
    bleSequence++;
    bleReadSample += count;
    available -= count;
  }
  if (finished && available == 0) {
    bleCapture = false;
    notifyBluetoothStatus();
    Serial.printf("[BLE] Capture ended: %lu samples, %lu PCM frames submitted.\n",
                  (unsigned long)bleReadSample, (unsigned long)bleSequence);
  }
}

#include "AuscultaBleV2.h"

// --------------------------------------------------
// SETUP
// --------------------------------------------------

void setup() {
  Serial.begin(115200);
#if ARDUINO_USB_CDC_ON_BOOT
  Serial0.begin(115200);
#endif
  delay(1000);
  bleLogf("[BOOT] BLE diagnostic build v2-trace-1 resetReason=%d\n", int(esp_reset_reason()));

  Serial.println("\n========================================");
  Serial.println("AUSCULTA STETHOSCOPE - AUTO-SCAN 8 ZONES");
  Serial.println("========================================");

  pinMode(MUX_S0, OUTPUT);
  pinMode(MUX_S1, OUTPUT);
  pinMode(MUX_S2, OUTPUT);

  selectMicrophone(1);

  audioBuffer = (int16_t*) ps_malloc(MAX_SAMPLES * sizeof(int16_t));

  if (!audioBuffer) {
    audioBuffer = (int16_t*) malloc(MAX_SAMPLES * sizeof(int16_t));
  }

  if (!audioBuffer) {
    Serial.println("[AUDIO] Failed to allocate audio buffer.");
  }

  initI2S();
  initSDCard();

  setupHighPass(highPass, FILTER_LOW_HZ, 0.707f);
  setupNotch(notch60, FILTER_NOTCH_HZ, 10.0f);
  setupLowPass(lowPass, FILTER_HIGH_HZ, 0.707f);

  resetFilters();

  delay(500);

  WiFi.mode(WIFI_AP);
  WiFi.softAP(AP_SSID, AP_PASSWORD);
  WiFi.setTxPower(WIFI_POWER_8_5dBm);

  // ADDED: Initialize Bluetooth after Wi-Fi setup
  initBluetooth();

  // --------------------------------------------------
  // WEB ROUTES
  // --------------------------------------------------

  server.on("/", HTTP_GET, []() {
    server.send_P(200, "text/html", MAIN_PAGE);
  });

  // ADDED: Bluetooth connection status route
  server.on("/ble-status", HTTP_GET, []() {

    String json = "{";

    json += "\"connected\":";
    json += (bleDeviceConnected ? "true" : "false");

    json += "}";

    server.send(200, "application/json", json);
  });

  server.on("/mic", HTTP_GET, []() {
    if (v2OwnsRecorder || v2BulkBusy) { server.send(409, "text/plain", "BLE v2 owns recorder"); return; }

    if (bleCapture || recording || recordingTaskRunning || scanningAll) {
      server.send(409, "text/plain", "Recording in progress");
      return;
    }

    if (server.hasArg("channel")) {
      int ch = server.arg("channel").toInt();

      selectMicrophone(ch);
      i2s_zero_dma_buffer(I2S_PORT);

      server.send(200, "text/plain",
                  "Selected " + sensorName(selectedMic));
    } else {
      server.send(400, "text/plain", "Missing channel");
    }
  });

  // SINGLE RECORDING
  server.on("/record/start", HTTP_GET, []() {
    if (v2OwnsRecorder || v2BulkBusy) { server.send(409, "text/plain", "BLE v2 owns recorder"); return; }

    if (bleCapture || recording || scanningAll || recordingTaskRunning) {
      server.send(409, "text/plain", "Busy");
      return;
    }

    if (!audioBuffer) {
      server.send(500, "text/plain", "Audio buffer unavailable");
      return;
    }

    if (server.hasArg("channel")) {
      selectMicrophone(server.arg("channel").toInt());
    }

    startRecording();

    if (!recording) {
      server.send(500, "text/plain", "Failed to start recording task");
      return;
    }

    server.send(200, "text/plain", "Started");
  });

  // START AUTO-SCAN
  server.on("/record/all", HTTP_GET, []() {
    if (v2OwnsRecorder || v2BulkBusy) { server.send(409, "text/plain", "BLE v2 owns recorder"); return; }

    if (bleCapture || recording || scanningAll || recordingTaskRunning) {
      server.send(409, "text/plain", "Busy");
      return;
    }

    if (!audioBuffer) {
      server.send(500, "text/plain", "Audio buffer unavailable");
      return;
    }

    currentScanFolder = createScanFolder();

    if (currentScanFolder.length() == 0) {
      server.send(500, "text/plain", "Failed to create scan folder");
      return;
    }

    stopRequested = false;
    currentScanSensor = 0;

    // ADDED: Reset timer states before starting a new scan
    scanResting = false;
    scanPhaseStart = 0;
    scanPhaseDuration = 0;

    scanningAll = true;

    BaseType_t result = xTaskCreatePinnedToCore(
      recordAllMicsTask,
      "ScanAllTask",
      8192,
      NULL,
      3,
      &scanTaskHandle,
      0
    );

    if (result != pdPASS) {
      scanningAll = false;
      scanTaskHandle = NULL;

      server.send(500, "text/plain", "Failed to create scan task");
      return;
    }

    server.send(200, "text/plain",
                "Automated 8-zone scan started: " + currentScanFolder);
  });

  // STOP SINGLE OR AUTO-SCAN
  server.on("/record/stop", HTTP_GET, []() {
    if (v2OwnsRecorder || v2BulkBusy) { server.send(409, "text/plain", "Stop the job through BLE v2"); return; }

    if (!recording && !scanningAll) {
      server.send(200, "text/plain", "No active recording");
      return;
    }

    stopRecording();

    server.send(200, "text/plain", "Stop requested");
  });

  // CURRENT STATUS
  // UPDATED: Includes recording elapsed time and rest countdown.
  server.on("/status", HTTP_GET, []() {

    String mode = "idle";
    String message = "Ready";

    uint32_t elapsed = 0;
    uint32_t remaining = 0;

    if (scanningAll) {

      mode = "scan";

      if (scanResting) {

        message = "Resting before Sensor " +
                  String(currentScanSensor + 1) + "...";

      } else if (currentScanSensor > 0) {

        message = "Recording Sensor " +
                  String(currentScanSensor) + " of 8...";

      } else {

        message = "Preparing scan...";
      }

      if (stopRequested) {
        message = "Stopping scan...";
      }

      if (scanPhaseDuration > 0) {

        uint32_t currentElapsed = millis() - scanPhaseStart;

        if (currentElapsed > scanPhaseDuration) {
          currentElapsed = scanPhaseDuration;
        }

        elapsed = currentElapsed;
        remaining = scanPhaseDuration - currentElapsed;
      }

    } else if (recording || recordingTaskRunning) {

      mode = "single";
      message = "Recording Single...";
    }

    String json = "{";

    json += "\"mode\":\"" + mode + "\",";
    json += "\"message\":\"" + message + "\",";
    json += "\"sensor\":" + String(currentScanSensor) + ",";
    json += "\"resting\":" + String(scanResting ? "true" : "false") + ",";
    json += "\"elapsed\":" + String(elapsed) + ",";
    json += "\"remaining\":" + String(remaining);

    json += "}";

    server.send(200, "application/json", json);
  });

  // LIST SINGLE RECORDINGS FROM ROOT
  server.on("/list", HTTP_GET, []() {
    if (v2OwnsRecorder || v2BulkBusy) { server.send(409, "text/plain", "BLE v2 saving/capturing"); return; }

    String json = "[";

    File root = SD.open("/");
    bool first = true;

    if (root) {
      File file = root.openNextFile();

      while (file) {

        String fileName = String(file.name());

        if (!fileName.startsWith("/")) {
          fileName = "/" + fileName;
        }

        if (!file.isDirectory() && fileName.endsWith(".wav")) {

          if (!first) json += ",";

          json += "\"" + fileName + "\"";
          first = false;
        }

        file.close();
        file = root.openNextFile();
      }

      root.close();
    }

    json += "]";

    server.send(200, "application/json", json);
  });

  // LIST SCAN FOLDERS
  server.on("/scans", HTTP_GET, []() {
    if (v2OwnsRecorder || v2BulkBusy) { server.send(409, "text/plain", "BLE v2 saving/capturing"); return; }

    String json = "[";

    File root = SD.open("/");
    bool first = true;

    if (root) {
      File file = root.openNextFile();

      while (file) {

        String folderName = String(file.name());

        if (!folderName.startsWith("/")) {
          folderName = "/" + folderName;
        }

        if (file.isDirectory() && folderName.startsWith("/Scan_")) {

          if (!first) json += ",";

          json += "\"" + folderName + "\"";
          first = false;
        }

        file.close();
        file = root.openNextFile();
      }

      root.close();
    }

    json += "]";

    server.send(200, "application/json", json);
  });

  // LIST WAV FILES INSIDE A SELECTED SCAN FOLDER
  server.on("/scanfiles", HTTP_GET, []() {
    if (v2OwnsRecorder || v2BulkBusy) { server.send(409, "text/plain", "BLE v2 saving/capturing"); return; }

    if (!server.hasArg("folder")) {
      server.send(400, "application/json", "[]");
      return;
    }

    String folder = server.arg("folder");

    // Only allow folders created by the scan feature.
    if (!folder.startsWith("/Scan_") || folder.indexOf("..") >= 0) {
      server.send(400, "application/json", "[]");
      return;
    }

    File root = SD.open(folder.c_str());

    String json = "[";
    bool first = true;

    if (root && root.isDirectory()) {

      File file = root.openNextFile();

      while (file) {

        String fileName = String(file.name());

        if (!fileName.startsWith("/")) {
          fileName = folder + "/" + fileName;
        } else if (fileName.indexOf(folder) != 0) {
          fileName = folder + fileName;
        }

        if (!file.isDirectory() && fileName.endsWith(".wav")) {

          if (!first) json += ",";

          json += "\"" + fileName + "\"";
          first = false;
        }

        file.close();
        file = root.openNextFile();
      }

      root.close();
    }

    json += "]";

    server.send(200, "application/json", json);
  });

  // PLAYBACK AND DOWNLOAD
  server.on("/audio", HTTP_GET, []() {
    if (v2Subscribed) { server.send(409, "text/plain", "BLE v2 session active; disconnect before HTTP playback"); return; }
    if (v2OwnsRecorder || v2BulkBusy) { server.send(409, "text/plain", "BLE v2 saving/capturing"); return; }
    if (bleCapture) {
      server.send(409, "text/plain", "BLE audio capture in progress");
      return;
    }

    const bool download =
      server.hasArg("download") && server.arg("download") == "1";

    if (download && (recording || recordingTaskRunning || scanningAll)) {
      server.send(409, "text/plain",
                  "Please wait until recording and saving finish, then download again.");
      return;
    }

    if (!server.hasArg("file")) {
      server.send(400, "text/plain", "Missing file parameter");
      return;
    }

    String filename = server.arg("file");

    if (!filename.startsWith("/")) {
      filename = "/" + filename;
    }

    if (filename.indexOf("..") >= 0) {
      server.send(400, "text/plain", "Invalid file path");
      return;
    }

    if (!SD.exists(filename.c_str())) {
      server.send(404, "text/plain", "File not found");
      return;
    }

    File audioFile = SD.open(filename.c_str(), FILE_READ);

    if (!audioFile) {
      server.send(500, "text/plain", "Failed to open file");
      return;
    }

    if (download) {

      String downloadName =
        filename.substring(filename.lastIndexOf('/') + 1);

      for (unsigned int i = 0; i < downloadName.length(); i++) {

        const char c = downloadName[i];

        if (!((c >= 'a' && c <= 'z') ||
              (c >= 'A' && c <= 'Z') ||
              (c >= '0' && c <= '9') ||
              c == '.' || c == '_' || c == '-' ||
              c == ' ' || c == '(' || c == ')')) {

          downloadName.setCharAt(i, '_');
        }
      }

      if (!downloadName.endsWith(".wav")) {
        downloadName += ".wav";
      }

      server.sendHeader(
        "Content-Disposition",
        "attachment; filename=\"" + downloadName + "\""
      );

      server.sendHeader("Cache-Control", "no-store");
    }

    server.streamFile(audioFile, "audio/wav");
    audioFile.close();
  });

  server.begin();

  Serial.println("Web server running at http://192.168.4.1");
}

// --------------------------------------------------
// MAIN LOOP
// --------------------------------------------------

void loop() {

  processV2Bluetooth();
  processBulkBluetooth();
  processBluetooth();
  if (!v2Tx.length()) server.handleClient();

  if (recordingFinished) {

    recordingFinished = false;

    Serial.printf(
      "[AUDIO STATS] Min: %d | Max: %d | RMS: %u\n",
      recordedMin,
      recordedMax,
      rmsLevel
    );
  }

  delay(1);
}


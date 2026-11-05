// Freestanding host tests: no hardware or Windows SDK libraries required.
#include "../BleV2Wire.h"
#include "../BleBulkWire.h"
#include <new>
extern "C" void* __cdecl memcpy(void* target, const void* source, size_t count) {
  unsigned char* out = (unsigned char*)target;
  const unsigned char* in = (const unsigned char*)source;
  while (count--) *out++ = *in++;
  return target;
}
extern "C" void* __cdecl memset(void* target, int value, size_t count) {
  unsigned char* out = (unsigned char*)target;
  while (count--) *out++ = (unsigned char)value;
  return target;
}
#define CHECK(condition) do { if (!(condition)) return __LINE__; } while (0)
alignas(BleV2Wire::Assembly) static uint8_t assemblyStorage[sizeof(BleV2Wire::Assembly)];
static uint8_t source[4096];
static uint8_t bulkPacket[512];
int bulkChecks() {
  using namespace BleBulkWire;
  TxTracker tracker;
  for (int i=0;i<8;++i) { tracker.begin(); tracker.result(0); tracker.event(0); }
  CHECK(tracker.accepted==8 && tracker.completed==8 && !tracker.pending);
  tracker.begin(); tracker.event(6); tracker.result(6);
  CHECK(tracker.attempt==9 && tracker.callbackAttempt==9);
  CHECK(tracker.errors==0 && tracker.accepted==8 && tracker.completed==8);
  CHECK(tracker.rejectedCallbacks==1 && !tracker.pending);
  tracker.begin(); tracker.event(0); tracker.result(0); // retry succeeds inline
  CHECK(tracker.accepted==9 && tracker.completed==9 && !tracker.pending);
  tracker.begin(); tracker.result(0); tracker.event(6); // accepted TX failure is fatal
  CHECK(tracker.errors==1 && tracker.lastError==6 && tracker.completed==10);
  tracker.event(6); CHECK(tracker.unownedCallbacks==1 && tracker.errors==1);
  TxTracker late;
  late.begin(); late.result(6); late.event(6);
  CHECK(late.accepted==0 && late.completed==0 && late.errors==0 && late.unownedCallbacks==1);
  late.begin(); late.result(0); late.event(0);
  CHECK(late.accepted==1 && late.completed==1 && late.errors==0);
  CHECK(valueBytes(23) == 20 && valueBytes(185) == 182);
  CHECK(valueBytes(517) == 512 && valueBytes(1024) == 512 && valueBytes(3) == 0);
  Window invalid;
  invalid.reset(100);
  CHECK(invalid.ack(0, 0) == Ack::Invalid && invalid.ack(0, 17) == Ack::Invalid);
  CHECK(invalid.ack(1, 1) == Ack::Invalid);
  CHECK(invalid.ack(0, 2) == Ack::Granted);
  CHECK(invalid.ack(0, 2) == Ack::Ignored && invalid.credits == 2);
  CHECK(invalid.submitted(20));
  CHECK(invalid.ack(20, 2) == Ack::Invalid); // cannot replenish mid-window
  CHECK(invalid.submitted(20));
  CHECK(!invalid.canSend());
  CHECK(invalid.ack(20, 2) == Ack::Invalid); // missing data, no credit granted
  CHECK(invalid.ack(0, 2) == Ack::Ignored && invalid.credits == 0);
  CHECK(invalid.ack(41, 2) == Ack::Invalid); // future ACK
  CHECK(invalid.ack(40, 1) == Ack::Granted);
  CHECK(!invalid.submitted(61) && invalid.sent == 40);
  CHECK(invalid.submitted(60));
  CHECK(invalid.ack(100, 1) == Ack::Complete);
  CHECK(invalid.ack(100, 1) == Ack::Ignored && !invalid.canSend());
  const unsigned mtus[] = {179,185,247,517};
  const unsigned grants[] = {1,4,16};
  for (unsigned m = 0; m < 4; ++m) for (unsigned g = 0; g < 3; ++g) {
    Window tx;
    tx.reset(640044);
    CHECK(tx.ack(0, grants[g]) == Ack::Granted);
    uint32_t received = 0, crc = 0xffffffffU, expectedCrc = 0xffffffffU;
    while (tx.canSend()) {
      unsigned count = valueBytes(mtus[m]) - HeaderBytes;
      if (count > tx.length - tx.sent) count = tx.length - tx.sent;
      header(bulkPacket, 73, tx.sent, 8, count, tx.sent + count == tx.length);
      for (unsigned i = 0; i < count; ++i) bulkPacket[HeaderBytes+i] = uint8_t((tx.sent+i)*31U);
      CHECK(read32(bulkPacket + 4) == 73 && read32(bulkPacket + 8) == received);
      CHECK(bulkPacket[12] == 8 && (bulkPacket[14] | (unsigned(bulkPacket[15]) << 8)) == count);
      CHECK(count + HeaderBytes <= mtus[m] - 3 && count + HeaderBytes <= 512);
      CHECK(bulkPacket[13] == (received + count == 640044 ? 1 : 0));
      crc = BleV2Wire::crcUpdate(crc, bulkPacket + HeaderBytes, count);
      received += count;
      CHECK(tx.submitted(count));
      if (tx.waiting()) {
        CHECK(tx.ack(received, grants[g]) == (received == 640044 ? Ack::Complete : Ack::Granted));
      }
    }
    CHECK(received == 640044 && tx.acknowledged == received);
    // Independent chunk boundaries must produce the same complete-file CRC.
    for (uint32_t offset = 0; offset < 640044;) {
      unsigned count = 640044 - offset > 64 ? 64 : 640044 - offset;
      for (unsigned i = 0; i < count; ++i) bulkPacket[i] = uint8_t((offset+i)*31U);
      expectedCrc = BleV2Wire::crcUpdate(expectedCrc, bulkPacket, count); offset += count;
    }
    CHECK(crc == expectedCrc);
  }
  return 0;
}
extern "C" int testEntry() {
  int bulkResult = bulkChecks();
  if (bulkResult) return bulkResult;
  using namespace BleV2Wire;
  SendWindow window;
  CHECK(!window.ready(15) && window.ready(16));
  window.accepted(16);
  CHECK(!window.ready(31) && window.ready(32));
  for (unsigned retry = 0; retry < 8; ++retry) {
    uint32_t at = 32 + retry * 32;
    CHECK(window.noMemory(at));
    CHECK(!window.ready(at + 31) && window.ready(at + 32));
  }
  CHECK(!window.noMemory(288)); // finite bound on rejected submissions
  window.accepted(300);
  CHECK(window.failures == 0 && window.ready(316));
  window.accepted(0xfffffff0U);
  CHECK(window.ready(0)); // millis rollover
  window.reset();
  CHECK(window.failures == 0 && window.gap == 16);
  Assembly& assembly = *new (assemblyStorage) Assembly;
  CHECK((crcUpdate(0xffffffffU, (const uint8_t*)"123456789", 9) ^ 0xffffffffU) == 0xcbf43926U);
  uint32_t crc = crcUpdate(0xffffffffU, (const uint8_t*)"1234", 4);
  CHECK((crcUpdate(crc, (const uint8_t*)"56789", 5) ^ 0xffffffffU) == 0xcbf43926U);
  CHECK((crcUpdate(0xffffffffU, source, 0) ^ 0xffffffffU) == 0);
  // Physical iPhone trace: no command may be dispatched after only 32/33 bytes.
  const uint8_t first[] = {0x21,0,0,0,0x7b,0x22,0x69,0x64,0x22,0x3a,0x31,0x2c,0x22,0x63,0x6d,0x64,0x22,0x3a,0x22,0x47};
  const uint8_t second[] = {0x21,0,0x10,0,0x45,0x54,0x5f,0x43,0x41,0x50,0x41,0x42,0x49,0x4c,0x49,0x54,0x49,0x45,0x53,0x22};
  const uint8_t third[] = {0x21,0,0x20,0,0x7d};
  assembly.reset();
  CHECK(assembly.add(first, sizeof(first)) == Feed::More);
  CHECK(assembly.used == 16 && assembly.total == 33);
  CHECK(assembly.add(second, sizeof(second)) == Feed::More);
  CHECK(assembly.used == 32 && assembly.total == 33);
  CHECK(assembly.add(third, sizeof(third)) == Feed::Complete);
  const char expected[] = "{\"id\":1,\"cmd\":\"GET_CAPABILITIES\"}";
  for (unsigned i = 0; i < sizeof(expected); ++i) CHECK(assembly.bytes[i] == expected[i]);
  for (unsigned i = 0; i < sizeof(source); ++i) source[i] = (uint8_t)(i * 31);
  const unsigned lengths[] = {1, 15, 16, 17, 31, 32, 33, 768, 1151, 2048, 4096};
  for (unsigned test = 0; test < sizeof(lengths)/sizeof(lengths[0]); ++test) {
    assembly.reset();
    unsigned length = lengths[test];
    for (unsigned offset = 0; offset < length;) {
      uint8_t frame[20];
      size_t count = fragment(source, length, offset, frame);
      CHECK(count >= 5 && count <= 20);
      Feed status = assembly.add(frame, count);
      offset += count - 4;
      CHECK(status == (offset == length ? Feed::Complete : Feed::More));
    }
    CHECK(assembly.used == length && assembly.total == length);
    for (unsigned i = 0; i < length; ++i) CHECK((uint8_t)assembly.bytes[i] == source[i]);
    CHECK(assembly.bytes[length] == 0);
  }
  uint8_t frame[20];
  assembly.reset();
  CHECK(assembly.add(frame, 4) == Feed::Invalid);
  CHECK(assembly.add(frame, 21) == Feed::Invalid);
  fragment(source, 17, 0, frame);
  CHECK(assembly.add(frame, 19) == Feed::Invalid); // short nonfinal fragment
  fragment(source, 17, 16, frame);
  CHECK(assembly.add(frame, 5) == Feed::Invalid); // missing first fragment
  fragment(source, 17, 0, frame);
  CHECK(assembly.add(frame, 20) == Feed::More);
  CHECK(assembly.add(frame, 20) == Feed::Invalid); // duplicate / out of order
  fragment(source, 17, 0, frame);
  CHECK(assembly.add(frame, 20) == Feed::More);
  fragment(source, 18, 16, frame);
  CHECK(assembly.add(frame, 6) == Feed::Invalid); // total changed mid-message
  frame[0] = 0; frame[1] = 0; frame[2] = 0; frame[3] = 0;
  CHECK(assembly.add(frame, 5) == Feed::Invalid);
  frame[0] = 1; frame[1] = 16; // 4097
  CHECK(assembly.add(frame, 5) == Feed::Invalid);
  frame[0] = 1; frame[1] = 0;
  CHECK(assembly.add(frame, 6) == Feed::Invalid); // payload exceeds total
  CHECK(fragment(source, 0, 0, frame) == 0);
  CHECK(fragment(source, 4097, 0, frame) == 0);
  CHECK(fragment(source, 16, 16, frame) == 0);
  return 0;
}

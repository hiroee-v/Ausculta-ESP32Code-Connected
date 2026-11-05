#line 1 "C:\\Users\\Jerome Vargas\\Downloads\\Ausculta_Recorder_with_SD_Card_Module_All_8_Mics_WORKING__1_\\tests\\ble_v2_wire_test.cpp"
// Freestanding host tests: no hardware or Windows SDK libraries required.
#include "../BleV2Wire.h"
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
static BleV2Wire::Assembly assembly;
static uint8_t source[4096];
extern "C" int testEntry() {
  using namespace BleV2Wire;
  CHECK((crcUpdate(0xffffffffU, (const uint8_t*)"123456789", 9) ^ 0xffffffffU) == 0xcbf43926U);
  uint32_t crc = crcUpdate(0xffffffffU, (const uint8_t*)"1234", 4);
  CHECK((crcUpdate(crc, (const uint8_t*)"56789", 5) ^ 0xffffffffU) == 0xcbf43926U);
  CHECK((crcUpdate(0xffffffffU, source, 0) ^ 0xffffffffU) == 0);
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

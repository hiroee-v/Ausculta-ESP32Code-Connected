#line 1 "C:\\Users\\Jerome Vargas\\Downloads\\Ausculta_Recorder_with_SD_Card_Module_All_8_Mics_WORKING__1_\\BleV2Wire.h"
#pragma once
#include <stdint.h>
#include <stddef.h>
#include <string.h>

// Portable wire primitives, shared by firmware and host protocol tests.
namespace BleV2Wire {
enum class Feed { Invalid, More, Complete };
struct Assembly {
  char bytes[4097] = {};
  uint16_t total = 0;
  uint16_t used = 0;
  void reset() { total = used = 0; bytes[0] = 0; }
  Feed add(const uint8_t* frame, size_t size) {
    if (size < 5 || size > 20) { reset(); return Feed::Invalid; }
    uint16_t length = frame[0] | (uint16_t(frame[1]) << 8);
    uint16_t offset = frame[2] | (uint16_t(frame[3]) << 8);
    size_t count = size - 4;
    if (!length || length > 4096 || offset != used ||
        (used && length != total) || offset + count > length ||
        (offset + count < length && count != 16)) {
      reset(); return Feed::Invalid;
    }
    total = length;
    memcpy(bytes + used, frame + 4, count);
    used += count;
    bytes[used] = 0;
    return used == total ? Feed::Complete : Feed::More;
  }
};

inline size_t fragment(const uint8_t* bytes, uint16_t total, uint16_t offset, uint8_t* frame) {
  if (!total || total > 4096 || offset >= total) return 0;
  size_t count = total - offset;
  if (count > 16) count = 16;
  frame[0] = total & 255; frame[1] = total >> 8;
  frame[2] = offset & 255; frame[3] = offset >> 8;
  memcpy(frame + 4, bytes + offset, count);
  return count + 4;
}

inline uint32_t crcUpdate(uint32_t crc, const uint8_t* bytes, size_t count) {
  while (count--) {
    crc ^= *bytes++;
    for (int bit = 0; bit < 8; ++bit)
      crc = (crc >> 1) ^ ((crc & 1) ? 0xedb88320U : 0);
  }
  return crc;
}
}

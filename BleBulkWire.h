#pragma once
#include <stdint.h>
#include <stddef.h>
#include <string.h>

namespace BleBulkWire {
// One native call at a time, with at most one accepted notification pending.
// A callback may occur inside notify_custom, before its return is known.
struct TxTracker {
  bool inCall=false, pending=false, deferred=false;
  int deferredCode=0, lastError=0;
  uint32_t attempt=0, callbackAttempt=0, accepted=0, completed=0, errors=0;
  uint32_t rejectedCallbacks=0, unownedCallbacks=0;
  void begin() { inCall=true; deferred=false; ++attempt; }
  void completion(int code) {
    if (!pending) { ++unownedCallbacks; return; }
    callbackAttempt=attempt; pending=false; ++completed;
    if (code) { ++errors; lastError=code; }
  }
  void event(int code) {
    if (inCall) { deferred=true; deferredCode=code; return; }
    completion(code);
  }
  void result(int code) {
    inCall=false;
    if (!code) {
      ++accepted; pending=true;
      if (deferred) completion(deferredCode);
    } else if (deferred) {
      ++rejectedCallbacks; callbackAttempt=attempt;
    }
    deferred=false;
  }
};
constexpr uint16_t HeaderBytes = 20;
constexpr uint16_t MaxValueBytes = 512;
constexpr uint16_t MaxWindow = 16;
inline uint16_t valueBytes(uint16_t mtu) {
  return mtu > 3 ? (mtu - 3 > MaxValueBytes ? MaxValueBytes : mtu - 3) : 0;
}
inline uint32_t read32(const uint8_t* p) {
  return uint32_t(p[0]) | (uint32_t(p[1]) << 8) | (uint32_t(p[2]) << 16) | (uint32_t(p[3]) << 24);
}
inline void write32(uint8_t* p, uint32_t value) {
  for (unsigned i = 0; i < 4; ++i) p[i] = uint8_t(value >> (i * 8));
}
inline void header(uint8_t* p, uint32_t handle, uint32_t offset, uint8_t sensor, uint16_t count, bool last) {
  memcpy(p, "ABW1", 4);
  write32(p + 4, handle); write32(p + 8, offset);
  p[12] = sensor; p[13] = last ? 1 : 0;
  p[14] = uint8_t(count); p[15] = uint8_t(count >> 8);
  p[16] = p[17] = p[18] = p[19] = 0;
}
enum class Ack { Invalid, Ignored, Granted, Complete };
struct Window {
  uint32_t length = 0, sent = 0, acknowledged = 0;
  uint16_t credits = 0;
  bool started = false;
  void reset(uint32_t bytes) { length = bytes; sent = acknowledged = 0; credits = 0; started = false; }
  bool waiting() const { return !credits || sent == length; }
  bool canSend() const { return credits && sent < length; }
  Ack ack(uint32_t next, uint16_t grant) {
    if (!grant || grant > MaxWindow || next > sent) return Ack::Invalid;
    // Duplicate cumulative ACKs never replenish a window or extend its deadline.
    if (started && next == acknowledged) return Ack::Ignored;
    if (!waiting() || next != sent) return Ack::Invalid;
    if (next == length) { acknowledged = next; credits = 0; return Ack::Complete; }
    acknowledged = next; credits = grant; started = true;
    return Ack::Granted;
  }
  bool submitted(uint16_t bytes) {
    if (!canSend() || !bytes || uint32_t(bytes) > length - sent) return false;
    sent += bytes; --credits; return true;
  }
};
}

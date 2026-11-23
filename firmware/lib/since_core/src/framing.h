// SPDX-License-Identifier: GPL-3.0-or-later
// BLE-Framing (docs/ble-protocol.md): 1 Flag-Byte + Nutzdaten pro Frame.
#pragma once

#include <cstddef>
#include <cstdint>
#include <string>
#include <vector>

namespace since {

constexpr uint8_t kFrameFirst = 0x01;
constexpr uint8_t kFrameLast = 0x02;
constexpr size_t kMaxMessageBytes = 4096;

// Zerlegt msg in Frames mit höchstens frameSize Bytes (inkl. Flag-Byte, frameSize ≥ 2).
std::vector<std::string> splitFrames(const std::string& msg, size_t frameSize);

class Reassembler {
 public:
  enum class Result { kIncomplete, kComplete, kError };
  // Ein FIRST-Frame verwirft eine angefangene Nachricht. Frames ohne vorheriges FIRST
  // und Nachrichten > kMaxMessageBytes ergeben kError.
  Result push(const uint8_t* data, size_t len);
  // Fertige Nachricht abholen (nach kComplete).
  std::string take();
  bool tooLarge() const { return tooLarge_; }

 private:
  std::string buf_;
  std::string done_;
  bool active_ = false;
  bool tooLarge_ = false;
};

}  // namespace since

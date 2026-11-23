// SPDX-License-Identifier: GPL-3.0-or-later
#include "framing.h"

namespace since {

std::vector<std::string> splitFrames(const std::string& msg, size_t frameSize) {
  std::vector<std::string> frames;
  if (frameSize < 2) return frames;
  const size_t chunk = frameSize - 1;
  size_t pos = 0;
  do {
    const size_t n = msg.size() - pos < chunk ? msg.size() - pos : chunk;
    uint8_t flags = 0;
    if (pos == 0) flags |= kFrameFirst;
    if (pos + n >= msg.size()) flags |= kFrameLast;
    std::string f(1, static_cast<char>(flags));
    f.append(msg, pos, n);
    frames.push_back(f);
    pos += n;
  } while (pos < msg.size());
  return frames;
}

Reassembler::Result Reassembler::push(const uint8_t* data, size_t len) {
  if (len == 0) return Result::kError;
  const uint8_t flags = data[0];
  if (flags & kFrameFirst) {
    buf_.clear();
    active_ = true;
    tooLarge_ = false;
  } else if (!active_) {
    return Result::kError;
  }
  if (buf_.size() + len - 1 > kMaxMessageBytes) {
    buf_.clear();
    active_ = false;
    tooLarge_ = true;
    return Result::kError;
  }
  buf_.append(reinterpret_cast<const char*>(data + 1), len - 1);
  if (flags & kFrameLast) {
    done_.swap(buf_);
    buf_.clear();
    active_ = false;
    return Result::kComplete;
  }
  return Result::kIncomplete;
}

std::string Reassembler::take() {
  std::string out;
  out.swap(done_);
  return out;
}

}  // namespace since

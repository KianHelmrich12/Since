// SPDX-License-Identifier: GPL-3.0-or-later
#include "setup.h"

#include <cstdio>

namespace since {

std::string deviceName(const uint8_t mac[6]) {
  char buf[16];
  std::snprintf(buf, sizeof buf, "since-%02X%02X", mac[4], mac[5]);
  return buf;
}

static uint32_t uniform(const RandomFn& rnd, uint32_t n) {
  // Rejection Sampling gegen Modulo-Bias
  const uint32_t limit = UINT32_MAX - (UINT32_MAX % n);
  uint32_t v;
  do {
    v = rnd();
  } while (v >= limit);
  return v % n;
}

std::string randomWifiPassword(const RandomFn& rnd) {
  static const char kAlphabet[] = "abcdefghjkmnpqrstuvwxyz23456789";
  const uint32_t n = sizeof(kAlphabet) - 1;
  std::string out;
  for (int i = 0; i < 10; ++i) out += kAlphabet[uniform(rnd, n)];
  return out;
}

std::string randomPasskey(const RandomFn& rnd, uint32_t* numeric) {
  const uint32_t v = uniform(rnd, 1000000);
  if (numeric != nullptr) *numeric = v;
  char buf[8];
  std::snprintf(buf, sizeof buf, "%06u", static_cast<unsigned>(v));
  return buf;
}

static std::string escapeQr(const std::string& s) {
  std::string out;
  for (char c : s) {
    if (c == '\\' || c == ';' || c == ',' || c == ':' || c == '"') out += '\\';
    out += c;
  }
  return out;
}

std::string wifiQrPayload(const std::string& ssid, const std::string& password) {
  if (password.empty()) return "WIFI:T:nopass;S:" + escapeQr(ssid) + ";;";
  return "WIFI:T:WPA;S:" + escapeQr(ssid) + ";P:" + escapeQr(password) + ";;";
}

}  // namespace since

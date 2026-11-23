// SPDX-License-Identifier: GPL-3.0-or-later
// Minimale Arduino-Nachbildung für den Host-Emulator: genug für Adafruit_GFX,
// U8g2_for_Adafruit_GFX und src/render.cpp. Keine Hardware-Funktionen.
#pragma once

#include <cmath>
#include <cstddef>
#include <cstdint>
#include <cstdlib>
#include <cstring>
#include <string>

#ifndef ARDUINO
#define ARDUINO 100  // Adafruit_GFX erwartet Arduino ≥ 1.0 (size_t write)
#endif
// ArduinoJson-Adapter für Arduino-Streams/Strings sind per -D im Emulator-Build abgeschaltet.

#ifndef radians
#define radians(deg) ((deg) * 0.017453292519943295)
#endif
#ifndef degrees
#define degrees(rad) ((rad) * 57.29577951308232)
#endif

#define PROGMEM
#define pgm_read_byte(addr) (*(const unsigned char*)(addr))
#define pgm_read_word(addr) (*(const unsigned short*)(addr))
#define pgm_read_dword(addr) (*(const unsigned long*)(addr))
#define pgm_read_pointer(addr) ((void*)*(addr))

typedef bool boolean;
typedef uint8_t byte;

class __FlashStringHelper;

class String {
 public:
  String(const char* s = "") : s_(s ? s : "") {}
  String(const std::string& s) : s_(s) {}
  const char* c_str() const { return s_.c_str(); }
  unsigned int length() const { return static_cast<unsigned int>(s_.size()); }
  char charAt(unsigned int i) const { return s_[i]; }
  char operator[](unsigned int i) const { return s_[i]; }

 private:
  std::string s_;
};

#include "Print.h"

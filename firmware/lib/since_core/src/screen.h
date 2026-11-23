// SPDX-License-Identifier: GPL-3.0-or-later
// Bildschirm-Modell: welche Texte zeigt das Gerät? Unabhängig von Fonts und Pixeln,
// damit Inhalte auf dem Host testbar sind. Das Rendering passiert in src/display.cpp.
#pragma once

#include <cstdint>
#include <string>

#include "battery.h"
#include "civil.h"
#include "config.h"

namespace since {

enum class ScreenKind {
  kStandard,
  kMinimal,
  kMilestone,      // Standard + Banner (Geburtstag, Tag 1000, Monatstag); Minimal behält kMinimal + banner
  kBatteryEmpty,   // „Bitte laden“
  kSetup,          // Hotspot-Daten + QR + BLE-Passkey
  kSetTime,        // RTC hat Strom verloren
  kWelcome,        // noch kein Ereignis konfiguriert
};

struct SetupInfo {
  std::string ssid;
  std::string password;  // leer = offenes WLAN
  std::string url = "http://192.168.4.1";
  std::string passkey;   // BLE, 6 Ziffern
};

struct ScreenInputs {
  Config config;
  Epoch nowUtc = 0;
  bool rtcLostPower = false;
  int batteryMv = 0;
  BatteryLevel battery = BatteryLevel::kOk;
  bool setupMode = false;
  SetupInfo setup;
};

struct ScreenModel {
  ScreenKind kind = ScreenKind::kWelcome;
  std::string title;     // groß (Name)
  std::string subtitle;  // Anlass
  std::string headline;  // „3 Jahre · 2 Monate · 14 Tage“
  std::string line1;     // „seit 12.03.2023, 08:15“
  std::string line2;     // Gesamttage/-wochen
  std::string banner;    // Meilenstein-Text
  std::string qr;        // QR-Payload (Setup)
  int batteryPercent = -1;  // -1 = kein Symbol
  bool batteryWarning = false;
};

ScreenModel buildScreen(const ScreenInputs& in);

// Stabiler 32-bit-Fingerprint (FNV-1a) über alle sichtbaren Inhalte. Akkuprozent geht
// in 20-%-Stufen ein, damit nicht jede Spannungsschwankung neu zeichnet.
uint32_t screenFingerprint(const ScreenModel& m);

}  // namespace since

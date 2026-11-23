// SPDX-License-Identifier: GPL-3.0-or-later
// Gerätekonfiguration: Modell, Defaults, Validierung, JSON und Schema-Migration.
// Gemeinsames Format für NVS, Webportal (/api/config) und BLE.
#pragma once

#include <string>
#include <vector>

#include "civil.h"

namespace since {

constexpr int kConfigSchemaVersion = 1;

// Grenzen (Bytes bzw. Zeichen, siehe validate())
constexpr size_t kMaxNameChars = 32;
constexpr size_t kMaxOccasionChars = 64;
constexpr size_t kMaxTzBytes = 64;
constexpr size_t kMaxSsidBytes = 32;
constexpr size_t kMinPasswordBytes = 8;
constexpr size_t kMaxPasswordBytes = 63;
constexpr int kMinIntervalMinutes = 10;
constexpr int kMaxIntervalMinutes = 1440;

enum class Layout { kStandard, kMinimal };

// kAuto: nur neu zeichnen, wenn sich der angezeigte Text ändert (ohne Uhrzeit-Anzeige
// 1× täglich nach Mitternacht). kInterval: fest alle intervalMinutes.
enum class UpdateMode { kAuto, kInterval };

struct Config {
  int schemaVersion = kConfigSchemaVersion;
  std::string name;
  std::string occasion;
  bool hasEvent = false;
  DateTime event{2000, 1, 1, 0, 0, 0};  // lokale Zeit in `tz`
  std::string tz = "CET-1CEST,M3.5.0,M10.5.0/3";
  Layout layout = Layout::kStandard;
  bool showTime = false;  // Stunden/Minuten anzeigen
  UpdateMode updateMode = UpdateMode::kAuto;
  int intervalMinutes = 60;
  std::string wifiSsid;
  std::string wifiPassword;  // nur im NVS, nie über die API ausgegeben
  bool ntp = false;
  bool portalSecure = true;  // Hotspot mit zufälligem WPA2-Passwort
};

Config defaultConfig();

struct ValidationIssue {
  std::string field;  // JSON-Pfad, z. B. "wifi.password"
  std::string code;   // z. B. "too_long", "invalid", "required"
};

std::vector<ValidationIssue> validate(const Config& c);

enum class ConfigStatus {
  kOk,
  kInvalidJson,         // kein gültiges JSON-Objekt
  kWrongType,           // Feld mit falschem Typ
  kUnsupportedVersion,  // schemaVersion neuer als diese Firmware
  kInvalid,             // Validierung fehlgeschlagen
};

struct ConfigResult {
  ConfigStatus status = ConfigStatus::kOk;
  std::vector<ValidationIssue> issues;
  bool ok() const { return status == ConfigStatus::kOk; }
};

// Wendet ein (auch partielles) JSON-Dokument auf *cfg an: Felder, die fehlen, bleiben
// erhalten. Ein fehlendes wifi.password lässt das gespeicherte Passwort unverändert.
// Ältere Schema-Versionen werden migriert. Bei Fehlern bleibt *cfg unverändert.
ConfigResult applyJson(const std::string& json, Config* cfg);

// includeSecrets=false (API): statt des Passworts nur "passwordSet": true/false.
// includeSecrets=true (NVS): vollständig.
std::string toJson(const Config& c, bool includeSecrets);

// "YYYY-MM-DDTHH:MM" (Sekunden optional) ↔ DateTime
bool parseIsoLocal(const std::string& s, DateTime* out);
std::string formatIsoLocal(const DateTime& dt);

// Anzahl Unicode-Codepoints; -1 bei ungültigem UTF-8
int utf8Length(const std::string& s);

const char* layoutName(Layout l);
const char* updateModeName(UpdateMode m);

}  // namespace since

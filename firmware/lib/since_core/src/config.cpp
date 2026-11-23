// SPDX-License-Identifier: GPL-3.0-or-later
#include "config.h"

#include <ArduinoJson.h>

#include <cstdio>
#include <cstring>

#include "tz.h"

namespace since {
namespace {

// Migrationen: Eintrag i hebt ein Dokument von Version i+1 auf i+2.
// Für v1 gibt es noch keine; neue Schritte hier anhängen und kConfigSchemaVersion erhöhen.
using Migration = void (*)(JsonObject doc);
const Migration kMigrations[] = {nullptr};
constexpr int kMigrationCount = 0;

bool migrate(JsonObject doc, int fromVersion) {
  for (int v = fromVersion; v < kConfigSchemaVersion; ++v) {
    const int idx = v - 1;
    if (idx < 0 || idx >= kMigrationCount || kMigrations[idx] == nullptr) return false;
    kMigrations[idx](doc);
  }
  doc["schemaVersion"] = kConfigSchemaVersion;
  return true;
}

bool parseLayout(const char* s, Layout* out) {
  if (std::strcmp(s, "standard") == 0) *out = Layout::kStandard;
  else if (std::strcmp(s, "minimal") == 0) *out = Layout::kMinimal;
  else return false;
  return true;
}

bool parseUpdateMode(const char* s, UpdateMode* out) {
  if (std::strcmp(s, "auto") == 0) *out = UpdateMode::kAuto;
  else if (std::strcmp(s, "interval") == 0) *out = UpdateMode::kInterval;
  else return false;
  return true;
}

struct Reader {
  ConfigResult* result;
  void wrongType(const char* field) {
    result->status = ConfigStatus::kWrongType;
    result->issues.push_back({field, "wrong_type"});
  }
  void invalid(const char* field) { result->issues.push_back({field, "invalid"}); }

  void str(JsonVariantConst v, const char* field, std::string* out) {
    if (v.isNull()) return;
    if (!v.is<const char*>()) return wrongType(field);
    *out = v.as<const char*>();
  }
  void boolean(JsonVariantConst v, const char* field, bool* out) {
    if (v.isNull()) return;
    if (!v.is<bool>()) return wrongType(field);
    *out = v.as<bool>();
  }
  void integer(JsonVariantConst v, const char* field, int* out) {
    if (v.isNull()) return;
    if (!v.is<int>()) return wrongType(field);
    *out = v.as<int>();
  }
};

}  // namespace

Config defaultConfig() { return Config(); }

int utf8Length(const std::string& s) {
  int count = 0;
  for (size_t i = 0; i < s.size();) {
    const unsigned char c = static_cast<unsigned char>(s[i]);
    size_t len;
    if (c < 0x80) len = 1;
    else if ((c >> 5) == 0x6) len = 2;
    else if ((c >> 4) == 0xE) len = 3;
    else if ((c >> 3) == 0x1E) len = 4;
    else return -1;
    if (i + len > s.size()) return -1;
    for (size_t k = 1; k < len; ++k) {
      if ((static_cast<unsigned char>(s[i + k]) >> 6) != 0x2) return -1;
    }
    i += len;
    ++count;
  }
  return count;
}

bool parseIsoLocal(const std::string& s, DateTime* out) {
  DateTime dt{0, 0, 0, 0, 0, 0};
  char tail = 0;
  int n = std::sscanf(s.c_str(), "%4d-%2d-%2dT%2d:%2d:%2d%c", &dt.year, &dt.month, &dt.day,
                      &dt.hour, &dt.minute, &dt.second, &tail);
  const bool withSeconds = n == 6 && s.size() == 19;
  const bool withoutSeconds = n == 5 && s.size() == 16;
  if (!withSeconds && !withoutSeconds) return false;
  if (withoutSeconds) dt.second = 0;
  if (!isValidDateTime(dt)) return false;
  *out = dt;
  return true;
}

std::string formatIsoLocal(const DateTime& dt) {
  char buf[24];
  std::snprintf(buf, sizeof buf, "%04d-%02d-%02dT%02d:%02d", dt.year, dt.month, dt.day, dt.hour,
                dt.minute);
  return buf;
}

const char* layoutName(Layout l) { return l == Layout::kMinimal ? "minimal" : "standard"; }

const char* updateModeName(UpdateMode m) {
  return m == UpdateMode::kInterval ? "interval" : "auto";
}

std::vector<ValidationIssue> validate(const Config& c) {
  std::vector<ValidationIssue> issues;
  const int nameLen = utf8Length(c.name);
  if (nameLen < 0) issues.push_back({"name", "invalid_utf8"});
  else if (static_cast<size_t>(nameLen) > kMaxNameChars) issues.push_back({"name", "too_long"});

  const int occLen = utf8Length(c.occasion);
  if (occLen < 0) issues.push_back({"occasion", "invalid_utf8"});
  else if (static_cast<size_t>(occLen) > kMaxOccasionChars)
    issues.push_back({"occasion", "too_long"});

  if (c.hasEvent && !isValidDateTime(c.event)) issues.push_back({"event", "invalid"});

  TimeZone tz;
  if (c.tz.size() > kMaxTzBytes) issues.push_back({"tz", "too_long"});
  else if (!TimeZone::parse(c.tz, &tz)) issues.push_back({"tz", "invalid"});

  if (c.updateMode == UpdateMode::kInterval &&
      (c.intervalMinutes < kMinIntervalMinutes || c.intervalMinutes > kMaxIntervalMinutes)) {
    issues.push_back({"update.intervalMinutes", "out_of_range"});
  }

  if (c.wifiSsid.size() > kMaxSsidBytes) issues.push_back({"wifi.ssid", "too_long"});
  if (!c.wifiPassword.empty() && (c.wifiPassword.size() < kMinPasswordBytes ||
                                  c.wifiPassword.size() > kMaxPasswordBytes)) {
    issues.push_back({"wifi.password", "length"});
  }
  if (c.ntp && c.wifiSsid.empty()) issues.push_back({"ntp", "requires_wifi"});
  return issues;
}

ConfigResult applyJson(const std::string& json, Config* cfg) {
  ConfigResult result;
  JsonDocument doc;
  if (deserializeJson(doc, json) != DeserializationError::Ok || !doc.is<JsonObject>()) {
    result.status = ConfigStatus::kInvalidJson;
    return result;
  }
  JsonObject root = doc.as<JsonObject>();

  int version = kConfigSchemaVersion;
  if (!root["schemaVersion"].isNull()) {
    if (!root["schemaVersion"].is<int>()) {
      result.status = ConfigStatus::kWrongType;
      result.issues.push_back({"schemaVersion", "wrong_type"});
      return result;
    }
    version = root["schemaVersion"].as<int>();
  }
  if (version > kConfigSchemaVersion || version < 1 || !migrate(root, version)) {
    result.status = ConfigStatus::kUnsupportedVersion;
    result.issues.push_back({"schemaVersion", "unsupported"});
    return result;
  }

  Config next = *cfg;
  Reader r{&result};
  r.str(root["name"], "name", &next.name);
  r.str(root["occasion"], "occasion", &next.occasion);

  JsonVariantConst ev = root["event"];
  if (!ev.isNull()) {
    if (!ev.is<const char*>()) {
      r.wrongType("event");
    } else if (std::strlen(ev.as<const char*>()) == 0) {
      next.hasEvent = false;
    } else if (parseIsoLocal(ev.as<const char*>(), &next.event)) {
      next.hasEvent = true;
    } else {
      r.invalid("event");
    }
  }

  r.str(root["tz"], "tz", &next.tz);

  JsonVariantConst layout = root["layout"];
  if (!layout.isNull()) {
    if (!layout.is<const char*>()) r.wrongType("layout");
    else if (!parseLayout(layout.as<const char*>(), &next.layout)) r.invalid("layout");
  }

  r.boolean(root["showTime"], "showTime", &next.showTime);

  JsonVariantConst update = root["update"];
  if (!update.isNull()) {
    if (!update.is<JsonObjectConst>()) {
      r.wrongType("update");
    } else {
      JsonVariantConst mode = update["mode"];
      if (!mode.isNull()) {
        if (!mode.is<const char*>()) r.wrongType("update.mode");
        else if (!parseUpdateMode(mode.as<const char*>(), &next.updateMode))
          r.invalid("update.mode");
      }
      r.integer(update["intervalMinutes"], "update.intervalMinutes", &next.intervalMinutes);
    }
  }

  JsonVariantConst wifi = root["wifi"];
  if (!wifi.isNull()) {
    if (!wifi.is<JsonObjectConst>()) {
      r.wrongType("wifi");
    } else {
      r.str(wifi["ssid"], "wifi.ssid", &next.wifiSsid);
      r.str(wifi["password"], "wifi.password", &next.wifiPassword);
    }
  }

  r.boolean(root["ntp"], "ntp", &next.ntp);
  r.boolean(root["portalSecure"], "portalSecure", &next.portalSecure);

  if (result.status == ConfigStatus::kWrongType) return result;
  if (!result.issues.empty()) {
    result.status = ConfigStatus::kInvalid;
    return result;
  }
  result.issues = validate(next);
  if (!result.issues.empty()) {
    result.status = ConfigStatus::kInvalid;
    return result;
  }
  next.schemaVersion = kConfigSchemaVersion;
  *cfg = next;
  return result;
}

std::string toJson(const Config& c, bool includeSecrets) {
  JsonDocument doc;
  doc["schemaVersion"] = kConfigSchemaVersion;
  doc["name"] = c.name;
  doc["occasion"] = c.occasion;
  doc["event"] = c.hasEvent ? formatIsoLocal(c.event) : std::string();
  doc["tz"] = c.tz;
  doc["layout"] = layoutName(c.layout);
  doc["showTime"] = c.showTime;
  JsonObject update = doc["update"].to<JsonObject>();
  update["mode"] = updateModeName(c.updateMode);
  update["intervalMinutes"] = c.intervalMinutes;
  JsonObject wifi = doc["wifi"].to<JsonObject>();
  wifi["ssid"] = c.wifiSsid;
  if (includeSecrets) wifi["password"] = c.wifiPassword;
  else wifi["passwordSet"] = !c.wifiPassword.empty();
  doc["ntp"] = c.ntp;
  doc["portalSecure"] = c.portalSecure;
  std::string out;
  serializeJson(doc, out);
  return out;
}

}  // namespace since

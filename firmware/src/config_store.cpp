// SPDX-License-Identifier: GPL-3.0-or-later
#include "config_store.h"

#include <Preferences.h>

namespace since {
namespace {
constexpr const char* kNamespace = "since";
constexpr const char* kKey = "cfg";
}  // namespace

Config loadConfig(bool* loaded) {
  Config cfg = defaultConfig();
  bool ok = false;
  Preferences prefs;
  if (prefs.begin(kNamespace, /*readOnly=*/true)) {
    if (prefs.isKey(kKey)) {
      const String json = prefs.getString(kKey, "");
      Config candidate = defaultConfig();
      // Migration älterer Schema-Versionen passiert in applyJson.
      if (applyJson(json.c_str(), &candidate).ok()) {
        cfg = candidate;
        ok = true;
      }
    }
    prefs.end();
  }
  if (loaded != nullptr) *loaded = ok;
  return cfg;
}

bool saveConfig(const Config& c) {
  if (!validate(c).empty()) return false;
  Preferences prefs;
  if (!prefs.begin(kNamespace, /*readOnly=*/false)) return false;
  const std::string json = toJson(c, /*includeSecrets=*/true);
  const size_t written = prefs.putString(kKey, json.c_str());
  prefs.end();
  return written == json.size();
}

bool eraseConfig() {
  Preferences prefs;
  if (!prefs.begin(kNamespace, /*readOnly=*/false)) return false;
  const bool ok = prefs.remove(kKey);
  prefs.end();
  return ok;
}

}  // namespace since

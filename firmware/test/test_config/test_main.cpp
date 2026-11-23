// SPDX-License-Identifier: GPL-3.0-or-later
#include <unity.h>

#include <dirent.h>

#include <fstream>
#include <sstream>
#include <string>
#include <vector>

#include "since_core.h"

using namespace since;

void setUp() {}
void tearDown() {}

static std::string readFile(const std::string& path) {
  std::ifstream f(path);
  std::stringstream ss;
  ss << f.rdbuf();
  return ss.str();
}

static std::vector<std::string> listJson(const std::string& dir) {
  std::vector<std::string> out;
  DIR* d = opendir(dir.c_str());
  if (d == nullptr) return out;
  while (dirent* e = readdir(d)) {
    const std::string n = e->d_name;
    if (n.size() > 5 && n.substr(n.size() - 5) == ".json") out.push_back(dir + "/" + n);
  }
  closedir(d);
  return out;
}

static const std::string kFixtures = std::string(SINCE_FIXTURES_DIR) + "/config";

static bool hasIssue(const ConfigResult& r, const char* field, const char* code) {
  for (const auto& i : r.issues)
    if (i.field == field && i.code == code) return true;
  return false;
}

static void test_defaults_are_valid() {
  const Config c = defaultConfig();
  TEST_ASSERT_TRUE(validate(c).empty());
  TEST_ASSERT_EQUAL(kConfigSchemaVersion, c.schemaVersion);
  TEST_ASSERT_FALSE(c.hasEvent);
  TEST_ASSERT_EQUAL_STRING(kDefaultTz, c.tz.c_str());
  TEST_ASSERT_TRUE(c.updateMode == UpdateMode::kAuto);
  TEST_ASSERT_FALSE(c.ntp);
  TEST_ASSERT_TRUE(c.portalSecure);
}

static void test_valid_fixtures_apply() {
  const auto files = listJson(kFixtures + "/valid");
  TEST_ASSERT_TRUE_MESSAGE(files.size() >= 3, "fixtures not found");
  for (const auto& f : files) {
    Config c = defaultConfig();
    const ConfigResult r = applyJson(readFile(f), &c);
    TEST_ASSERT_TRUE_MESSAGE(r.ok(), f.c_str());
  }
}

static void test_invalid_fixtures_rejected_unchanged() {
  const auto files = listJson(kFixtures + "/invalid");
  TEST_ASSERT_TRUE_MESSAGE(files.size() >= 8, "fixtures not found");
  for (const auto& f : files) {
    Config c = defaultConfig();
    c.name = "Unverändert";
    const ConfigResult r = applyJson(readFile(f), &c);
    TEST_ASSERT_FALSE_MESSAGE(r.ok(), f.c_str());
    TEST_ASSERT_EQUAL_STRING_MESSAGE("Unverändert", c.name.c_str(), f.c_str());
  }
}

static void test_full_fixture_values() {
  Config c = defaultConfig();
  TEST_ASSERT_TRUE(applyJson(readFile(kFixtures + "/valid/full.json"), &c).ok());
  TEST_ASSERT_EQUAL_STRING("Mila", c.name.c_str());
  TEST_ASSERT_TRUE(c.hasEvent);
  TEST_ASSERT_EQUAL(2023, c.event.year);
  TEST_ASSERT_EQUAL(15, c.event.minute);
  TEST_ASSERT_EQUAL_STRING("Heimnetz", c.wifiSsid.c_str());
  TEST_ASSERT_EQUAL_STRING("geheim123", c.wifiPassword.c_str());
  TEST_ASSERT_TRUE(c.ntp);
  TEST_ASSERT_FALSE(c.portalSecure);
}

static void test_round_trip_with_secrets() {
  Config a = defaultConfig();
  TEST_ASSERT_TRUE(applyJson(readFile(kFixtures + "/valid/full.json"), &a).ok());
  Config b = defaultConfig();
  TEST_ASSERT_TRUE(applyJson(toJson(a, true), &b).ok());
  TEST_ASSERT_EQUAL_STRING(toJson(a, true).c_str(), toJson(b, true).c_str());
  TEST_ASSERT_EQUAL_STRING("geheim123", b.wifiPassword.c_str());
}

static void test_round_trip_all_fixtures() {
  for (const auto& f : listJson(kFixtures + "/valid")) {
    Config a = defaultConfig();
    TEST_ASSERT_TRUE(applyJson(readFile(f), &a).ok());
    Config b = defaultConfig();
    TEST_ASSERT_TRUE_MESSAGE(applyJson(toJson(a, true), &b).ok(), f.c_str());
    TEST_ASSERT_EQUAL_STRING_MESSAGE(toJson(a, true).c_str(), toJson(b, true).c_str(), f.c_str());
  }
}

static void test_api_json_hides_password() {
  Config c = defaultConfig();
  TEST_ASSERT_TRUE(applyJson(readFile(kFixtures + "/valid/full.json"), &c).ok());
  const std::string api = toJson(c, false);
  TEST_ASSERT_TRUE(api.find("geheim123") == std::string::npos);
  TEST_ASSERT_TRUE(api.find("\"passwordSet\":true") != std::string::npos);
  // API-JSON zurückschreiben lässt das Passwort unverändert
  TEST_ASSERT_TRUE(applyJson(api, &c).ok());
  TEST_ASSERT_EQUAL_STRING("geheim123", c.wifiPassword.c_str());
}

static void test_partial_update_merges() {
  Config c = defaultConfig();
  TEST_ASSERT_TRUE(applyJson(readFile(kFixtures + "/valid/full.json"), &c).ok());
  TEST_ASSERT_TRUE(applyJson("{\"name\":\"Ben\"}", &c).ok());
  TEST_ASSERT_EQUAL_STRING("Ben", c.name.c_str());
  TEST_ASSERT_EQUAL_STRING("seit deiner Geburt", c.occasion.c_str());
  TEST_ASSERT_TRUE(c.hasEvent);
  // leeres event löscht das Ereignis
  TEST_ASSERT_TRUE(applyJson("{\"event\":\"\"}", &c).ok());
  TEST_ASSERT_FALSE(c.hasEvent);
}

static void test_status_codes() {
  Config c = defaultConfig();
  TEST_ASSERT_TRUE(applyJson("[1,2]", &c).status == ConfigStatus::kInvalidJson);
  TEST_ASSERT_TRUE(applyJson("{", &c).status == ConfigStatus::kInvalidJson);
  TEST_ASSERT_TRUE(applyJson("{\"name\":1}", &c).status == ConfigStatus::kWrongType);
  TEST_ASSERT_TRUE(applyJson("{\"schemaVersion\":2}", &c).status ==
                   ConfigStatus::kUnsupportedVersion);
  TEST_ASSERT_TRUE(applyJson("{\"schemaVersion\":0}", &c).status ==
                   ConfigStatus::kUnsupportedVersion);
  TEST_ASSERT_TRUE(applyJson("{\"layout\":\"riesig\"}", &c).status == ConfigStatus::kInvalid);
}

static void test_validation_issue_fields() {
  Config c = defaultConfig();
  ConfigResult r = applyJson("{\"wifi\":{\"ssid\":\"N\",\"password\":\"kurz\"}}", &c);
  TEST_ASSERT_TRUE(hasIssue(r, "wifi.password", "length"));
  r = applyJson("{\"ntp\":true}", &c);
  TEST_ASSERT_TRUE(hasIssue(r, "ntp", "requires_wifi"));
  r = applyJson("{\"update\":{\"mode\":\"interval\",\"intervalMinutes\":2000}}", &c);
  TEST_ASSERT_TRUE(hasIssue(r, "update.intervalMinutes", "out_of_range"));
  r = applyJson("{\"tz\":\"Mars\"}", &c);
  TEST_ASSERT_TRUE(hasIssue(r, "tz", "invalid"));
}

static void test_interval_ignored_in_auto_mode() {
  Config c = defaultConfig();
  TEST_ASSERT_TRUE(applyJson("{\"update\":{\"mode\":\"auto\",\"intervalMinutes\":1}}", &c).ok());
}

static void test_name_length_counts_codepoints() {
  Config c = defaultConfig();
  // 32 × „Ä“ = 64 Bytes, aber 32 Zeichen → gültig
  std::string name;
  for (int i = 0; i < 32; ++i) name += "Ä";
  c.name = name;
  TEST_ASSERT_TRUE(validate(c).empty());
  c.name += "x";
  TEST_ASSERT_FALSE(validate(c).empty());
  c.name = "\xC3";  // abgeschnittenes UTF-8
  TEST_ASSERT_EQUAL_STRING("invalid_utf8", validate(c)[0].code.c_str());
}

static void test_displayable_chars() {
  TEST_ASSERT_TRUE(isDisplayable("Jörg & Änne · ß"));
  TEST_ASSERT_TRUE(isDisplayable("Zoë »Mila«"));
  TEST_ASSERT_FALSE(isDisplayable("Łukasz"));             // U+0141, nicht Latin-1
  TEST_ASSERT_FALSE(isDisplayable("Mila \xF0\x9F\x8E\x82"));  // Emoji
  TEST_ASSERT_FALSE(isDisplayable("a\tb"));              // Steuerzeichen
  Config c = defaultConfig();
  const ConfigResult r = applyJson("{\"name\":\"Łukasz\"}", &c);
  TEST_ASSERT_TRUE(hasIssue(r, "name", "unsupported_char"));
}

static void test_utf8_length() {
  TEST_ASSERT_EQUAL(0, utf8Length(""));
  TEST_ASSERT_EQUAL(4, utf8Length("Jörg"));
  TEST_ASSERT_EQUAL(1, utf8Length("\xF0\x9F\x8E\x82"));  // 🎂
  TEST_ASSERT_EQUAL(-1, utf8Length("\xFF"));
}

static void test_iso_parse_format() {
  DateTime dt{};
  TEST_ASSERT_TRUE(parseIsoLocal("2023-03-12T08:15", &dt));
  TEST_ASSERT_EQUAL_STRING("2023-03-12T08:15", formatIsoLocal(dt).c_str());
  TEST_ASSERT_TRUE(parseIsoLocal("2023-03-12T08:15:30", &dt));
  TEST_ASSERT_EQUAL(30, dt.second);
  TEST_ASSERT_FALSE(parseIsoLocal("2023-3-12T08:15", &dt));
  TEST_ASSERT_FALSE(parseIsoLocal("2023-03-12 08:15", &dt));
  TEST_ASSERT_FALSE(parseIsoLocal("2023-03-12T25:00", &dt));
  TEST_ASSERT_FALSE(parseIsoLocal("2023-03-12T08:15Z", &dt));
  // streng: keine Vorzeichen oder Leerzeichen (wie die App)
  TEST_ASSERT_FALSE(parseIsoLocal("2026-03-12T08:15:+3", &dt));
  TEST_ASSERT_FALSE(parseIsoLocal("2026-03- 2T08:15", &dt));
  TEST_ASSERT_FALSE(parseIsoLocal("2026-+3-12T08:15", &dt));
  // Sekunden bleiben beim Speichern erhalten
  TEST_ASSERT_TRUE(parseIsoLocal("2023-03-12T08:15:30", &dt));
  TEST_ASSERT_EQUAL_STRING("2023-03-12T08:15:30", formatIsoLocal(dt).c_str());
}

static void test_wifi_password_follows_ssid() {
  Config c = defaultConfig();
  TEST_ASSERT_TRUE(applyJson("{\"wifi\":{\"ssid\":\"A\",\"password\":\"geheim123\"}}", &c).ok());
  // gleiche SSID ohne Passwort: bleibt
  TEST_ASSERT_TRUE(applyJson("{\"wifi\":{\"ssid\":\"A\"}}", &c).ok());
  TEST_ASSERT_EQUAL_STRING("geheim123", c.wifiPassword.c_str());
  // anderes Netz ohne Passwort: altes Passwort wird verworfen
  TEST_ASSERT_TRUE(applyJson("{\"wifi\":{\"ssid\":\"Offen\"}}", &c).ok());
  TEST_ASSERT_TRUE(c.wifiPassword.empty());
  // "" löscht ausdrücklich
  TEST_ASSERT_TRUE(applyJson("{\"wifi\":{\"ssid\":\"Offen\",\"password\":\"geheim123\"}}", &c).ok());
  TEST_ASSERT_TRUE(applyJson("{\"wifi\":{\"password\":\"\"}}", &c).ok());
  TEST_ASSERT_TRUE(c.wifiPassword.empty());
}

int main(int, char**) {
  UNITY_BEGIN();
  RUN_TEST(test_defaults_are_valid);
  RUN_TEST(test_valid_fixtures_apply);
  RUN_TEST(test_invalid_fixtures_rejected_unchanged);
  RUN_TEST(test_full_fixture_values);
  RUN_TEST(test_round_trip_with_secrets);
  RUN_TEST(test_round_trip_all_fixtures);
  RUN_TEST(test_api_json_hides_password);
  RUN_TEST(test_partial_update_merges);
  RUN_TEST(test_status_codes);
  RUN_TEST(test_validation_issue_fields);
  RUN_TEST(test_interval_ignored_in_auto_mode);
  RUN_TEST(test_name_length_counts_codepoints);
  RUN_TEST(test_displayable_chars);
  RUN_TEST(test_utf8_length);
  RUN_TEST(test_iso_parse_format);
  RUN_TEST(test_wifi_password_follows_ssid);
  return UNITY_END();
}

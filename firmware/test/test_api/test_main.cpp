// SPDX-License-Identifier: GPL-3.0-or-later
// Gemeinsame API (HTTP-Portal + BLE-RPC) gegen ein Fake-Gerät
#include <unity.h>

#include <ArduinoJson.h>

#include "since_core.h"

using namespace since;

class FakeDevice : public DeviceHooks {
 public:
  Config cfg = defaultConfig();
  Epoch time = 0;
  bool redraw = false, exitSetup = false, failSave = false;
  int touches = 0;
  Config loadConfig() override { return cfg; }
  bool saveConfig(const Config& c) override {
    if (failSave) return false;
    cfg = c;
    return true;
  }
  bool setTime(Epoch t) override {
    time = t;
    return true;
  }
  DeviceStatus status() override {
    DeviceStatus s;
    s.fw = "1.2.3";
    s.name = "since-AB12";
    s.batteryMv = 3900;
    s.batteryPercent = 62;
    s.nowUtc = time;
    return s;
  }
  void requestRedraw() override { redraw = true; }
  void requestExitSetup() override { exitSetup = true; }
  void touch() override { ++touches; }
};

static FakeDevice* dev;

void setUp() { dev = new FakeDevice(); }
void tearDown() { delete dev; }

static JsonDocument parse(const std::string& s) {
  JsonDocument d;
  TEST_ASSERT_TRUE(deserializeJson(d, s) == DeserializationError::Ok);
  return d;
}

static void test_get_info_and_status() {
  JsonDocument d = parse(callApi(*dev, "getInfo", "{}").body);
  TEST_ASSERT_EQUAL(1, d["proto"].as<int>());
  TEST_ASSERT_EQUAL_STRING("since-AB12", d["name"]);
  d = parse(callApi(*dev, "getStatus", "{}").body);
  TEST_ASSERT_EQUAL(3900, d["batteryMv"].as<int>());
  TEST_ASSERT_TRUE(d["charging"].isNull());
  TEST_ASSERT_TRUE(dev->touches >= 2);
}

static void test_set_config_ok_and_redraw() {
  const ApiResponse r = callApi(*dev, "setConfig", "{\"name\":\"Ben\",\"event\":\"2024-01-02T03:04\"}");
  TEST_ASSERT_TRUE(r.ok);
  TEST_ASSERT_EQUAL(200, r.httpStatus());
  TEST_ASSERT_EQUAL_STRING("Ben", dev->cfg.name.c_str());
  TEST_ASSERT_TRUE(dev->redraw);
}

static void test_set_config_invalid_lists_issues() {
  const ApiResponse r = callApi(*dev, "setConfig", "{\"wifi\":{\"ssid\":\"x\",\"password\":\"1\"}}");
  TEST_ASSERT_FALSE(r.ok);
  TEST_ASSERT_EQUAL(400, r.httpStatus());
  JsonDocument d = parse(r.body);
  TEST_ASSERT_EQUAL_STRING("invalid", d["error"]);
  TEST_ASSERT_EQUAL_STRING("wifi.password", d["issues"][0]["field"]);
  TEST_ASSERT_TRUE(dev->cfg.wifiSsid.empty());
}

static void test_set_config_storage_error() {
  dev->failSave = true;
  const ApiResponse r = callApi(*dev, "setConfig", "{\"name\":\"X\"}");
  TEST_ASSERT_EQUAL(500, r.httpStatus());
}

static void test_set_time() {
  TEST_ASSERT_TRUE(callApi(*dev, "setTime", "{\"epoch\":1795000000}").ok);
  TEST_ASSERT_EQUAL_INT64(1795000000, dev->time);
  TEST_ASSERT_EQUAL_STRING("bad_request", callApi(*dev, "setTime", "{\"epoch\":5}").error.c_str());
  TEST_ASSERT_EQUAL_STRING("bad_request", callApi(*dev, "setTime", "{}").error.c_str());
}

static void test_unknown_op() {
  const ApiResponse r = callApi(*dev, "factoryReset", "{}");
  TEST_ASSERT_EQUAL_STRING("unknown_op", r.error.c_str());
  TEST_ASSERT_EQUAL(404, r.httpStatus());
}

static void test_rpc_roundtrip() {
  JsonDocument d = parse(handleRpc(*dev, "{\"id\":7,\"op\":\"setConfig\",\"data\":{\"name\":\"Ida\"}}"));
  TEST_ASSERT_EQUAL(7, d["id"].as<int>());
  TEST_ASSERT_TRUE(d["ok"].as<bool>());
  TEST_ASSERT_EQUAL_STRING("Ida", d["data"]["name"]);
  d = parse(handleRpc(*dev, "{\"id\":8,\"op\":\"setTime\",\"epoch\":1795000000}"));
  TEST_ASSERT_TRUE(d["ok"].as<bool>());
  d = parse(handleRpc(*dev, "{\"id\":9,\"op\":\"setConfig\",\"data\":{\"tz\":\"Mars\"}}"));
  TEST_ASSERT_FALSE(d["ok"].as<bool>());
  TEST_ASSERT_EQUAL_STRING("invalid", d["error"]);
  TEST_ASSERT_EQUAL_STRING("tz", d["issues"][0]["field"]);
  d = parse(handleRpc(*dev, "{\"id\":10,\"op\":\"exitSetup\"}"));
  TEST_ASSERT_TRUE(dev->exitSetup);
}

static void test_rpc_bad_request() {
  JsonDocument d = parse(handleRpc(*dev, "kein json"));
  TEST_ASSERT_EQUAL_STRING("bad_request", d["error"]);
  d = parse(handleRpc(*dev, "{\"op\":\"getInfo\"}"));
  TEST_ASSERT_EQUAL_STRING("bad_request", d["error"]);
  d = parse(handleRpc(*dev, "{\"id\":1,\"op\":\"setConfig\"}"));
  TEST_ASSERT_EQUAL_STRING("invalid_json", d["error"]);
}

static void test_get_config_hides_password() {
  dev->cfg.wifiSsid = "Netz";
  dev->cfg.wifiPassword = "geheim123";
  const std::string body = callApi(*dev, "getConfig", "{}").body;
  TEST_ASSERT_TRUE(body.find("geheim123") == std::string::npos);
  TEST_ASSERT_TRUE(parse(body)["wifi"]["passwordSet"].as<bool>());
}

int main(int, char**) {
  UNITY_BEGIN();
  RUN_TEST(test_get_info_and_status);
  RUN_TEST(test_set_config_ok_and_redraw);
  RUN_TEST(test_set_config_invalid_lists_issues);
  RUN_TEST(test_set_config_storage_error);
  RUN_TEST(test_set_time);
  RUN_TEST(test_unknown_op);
  RUN_TEST(test_rpc_roundtrip);
  RUN_TEST(test_rpc_bad_request);
  RUN_TEST(test_get_config_hides_password);
  return UNITY_END();
}

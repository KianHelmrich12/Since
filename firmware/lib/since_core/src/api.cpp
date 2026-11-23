// SPDX-License-Identifier: GPL-3.0-or-later
#include "api.h"

#include <ArduinoJson.h>

namespace since {
namespace {

// Gültiger Bereich für gesetzte Zeit: 2024-01-01 … 2100-01-01
constexpr Epoch kMinEpoch = 1704067200;
constexpr Epoch kMaxEpoch = 4102444800;

ApiResponse error(const char* code, const std::vector<ValidationIssue>* issues = nullptr) {
  ApiResponse r;
  r.ok = false;
  r.error = code;
  JsonDocument doc;
  doc["error"] = code;
  if (issues != nullptr) {
    JsonArray arr = doc["issues"].to<JsonArray>();
    for (const auto& i : *issues) {
      JsonObject o = arr.add<JsonObject>();
      o["field"] = i.field;
      o["code"] = i.code;
    }
  }
  serializeJson(doc, r.body);
  return r;
}

ApiResponse okBody(std::string body) {
  ApiResponse r;
  r.body = std::move(body);
  return r;
}

const char* statusCode(ConfigStatus s) {
  switch (s) {
    case ConfigStatus::kInvalidJson:
      return "invalid_json";
    case ConfigStatus::kWrongType:
      return "wrong_type";
    case ConfigStatus::kUnsupportedVersion:
      return "unsupported_version";
    case ConfigStatus::kInvalid:
      return "invalid";
    case ConfigStatus::kOk:
    default:
      return "ok";
  }
}

}  // namespace

int ApiResponse::httpStatus() const {
  if (ok) return 200;
  if (error == "unknown_op") return 404;
  if (error == "too_large") return 413;
  if (error == "rtc_error" || error == "storage_error") return 500;
  return 400;
}

std::string statusJson(const DeviceStatus& s) {
  JsonDocument doc;
  doc["fw"] = s.fw;
  doc["name"] = s.name;
  doc["batteryMv"] = s.batteryMv;
  doc["batteryPercent"] = s.batteryPercent;
  if (s.charging < 0) doc["charging"] = nullptr;
  else doc["charging"] = s.charging == 1;
  doc["rtcOk"] = s.rtcOk;
  doc["rtcLostPower"] = s.rtcLostPower;
  doc["nowUtc"] = s.nowUtc;
  doc["lastSync"] = s.lastSync;
  doc["setupRemainingS"] = s.setupRemainingS;
  std::string out;
  serializeJson(doc, out);
  return out;
}

ApiResponse callApi(DeviceHooks& dev, const std::string& op, const std::string& paramsJson) {
  dev.touch();
  if (op == "getInfo") {
    const DeviceStatus st = dev.status();
    JsonDocument doc;
    doc["proto"] = kBleProtocolVersion;
    doc["fw"] = st.fw;
    doc["name"] = st.name;
    std::string out;
    serializeJson(doc, out);
    return okBody(out);
  }
  if (op == "getConfig") return okBody(toJson(dev.loadConfig(), false));
  if (op == "setConfig") {
    Config cfg = dev.loadConfig();
    const ConfigResult res = applyJson(paramsJson, &cfg);
    if (!res.ok()) return error(statusCode(res.status), &res.issues);
    if (!dev.saveConfig(cfg)) return error("storage_error");
    dev.requestRedraw();
    return okBody(toJson(cfg, false));
  }
  if (op == "setTime") {
    JsonDocument doc;
    if (deserializeJson(doc, paramsJson) != DeserializationError::Ok ||
        !doc["epoch"].is<int64_t>()) {
      return error("bad_request");
    }
    const Epoch epoch = doc["epoch"].as<int64_t>();
    if (epoch < kMinEpoch || epoch > kMaxEpoch) return error("bad_request");
    if (!dev.setTime(epoch)) return error("rtc_error");
    dev.requestRedraw();
    JsonDocument out;
    out["epoch"] = epoch;
    std::string body;
    serializeJson(out, body);
    return okBody(body);
  }
  if (op == "getStatus") return okBody(statusJson(dev.status()));
  if (op == "redraw") {
    dev.requestRedraw();
    return okBody("{}");
  }
  if (op == "exitSetup") {
    dev.requestExitSetup();
    return okBody("{}");
  }
  return error("unknown_op");
}

std::string handleRpc(DeviceHooks& dev, const std::string& request) {
  JsonDocument req;
  JsonDocument resp;
  if (deserializeJson(req, request) != DeserializationError::Ok || !req.is<JsonObject>() ||
      !req["id"].is<int>() || !req["op"].is<const char*>()) {
    resp["id"] = nullptr;
    resp["ok"] = false;
    resp["error"] = "bad_request";
    std::string out;
    serializeJson(resp, out);
    return out;
  }
  const int id = req["id"].as<int>();
  const std::string op = req["op"].as<const char*>();
  std::string params = "{}";
  if (op == "setConfig") {
    if (!req["data"].is<JsonObject>()) params = "null";
    else {
      params.clear();
      serializeJson(req["data"], params);
    }
  } else if (op == "setTime") {
    JsonDocument p;
    p["epoch"] = req["epoch"];
    params.clear();
    serializeJson(p, params);
  }

  const ApiResponse r = callApi(dev, op, params);
  resp["id"] = id;
  resp["ok"] = r.ok;
  JsonDocument body;
  deserializeJson(body, r.body);
  if (r.ok) {
    resp["data"] = body;
  } else {
    resp["error"] = r.error;
    if (!body["issues"].isNull()) resp["issues"] = body["issues"];
  }
  std::string out;
  serializeJson(resp, out);
  return out;
}

}  // namespace since

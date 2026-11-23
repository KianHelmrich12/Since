// SPDX-License-Identifier: GPL-3.0-or-later
#include "ble.h"

#include <Arduino.h>
#include <NimBLEDevice.h>

#include <deque>
#include <mutex>

#include "api.h"
#include "device.h"
#include "framing.h"

namespace ble {
namespace {

constexpr const char* kServiceUuid = "acdc0440-c5bd-4d15-815e-bced996cd9f9";
constexpr const char* kRxUuid = "c45a9546-5845-44f9-899a-844c4eaba12d";
constexpr const char* kTxUuid = "b4bc784e-edea-420a-9637-108af3fab72a";

NimBLEServer* g_server = nullptr;
NimBLECharacteristic* g_tx = nullptr;
since::Reassembler g_rx;
std::mutex g_mutex;
std::deque<std::string> g_pending;  // vollständige Anfragen
uint16_t g_conn = BLE_HS_CONN_HANDLE_NONE;  // aktuelle Verbindung (eine zur Zeit)

class ServerCallbacks : public NimBLEServerCallbacks {
  void onConnect(NimBLEServer*, NimBLEConnInfo& info) override {
    g_conn = info.getConnHandle();
    device::hooks().touch();
  }
  void onDisconnect(NimBLEServer*, NimBLEConnInfo&, int) override {
    g_conn = BLE_HS_CONN_HANDLE_NONE;
    NimBLEDevice::startAdvertising();
  }
  uint32_t onPassKeyDisplay() override { return device::setup().passkeyNumeric; }
  void onAuthenticationComplete(NimBLEConnInfo& info) override {
    if (!info.isEncrypted() || !info.isAuthenticated()) {
      NimBLEDevice::getServer()->disconnect(info.getConnHandle());
    }
  }
};

class RxCallbacks : public NimBLECharacteristicCallbacks {
  void onWrite(NimBLECharacteristic* c, NimBLEConnInfo&) override {
    const NimBLEAttValue v = c->getValue();
    const auto res = g_rx.push(v.data(), v.length());
    std::lock_guard<std::mutex> lock(g_mutex);
    if (res == since::Reassembler::Result::kComplete) {
      g_pending.push_back(g_rx.take());
    } else if (res == since::Reassembler::Result::kError && g_rx.tooLarge()) {
      g_pending.push_back("\x01too_large");
    }
  }
};

ServerCallbacks g_serverCb;
RxCallbacks g_rxCb;

void send(const std::string& msg) {
  if (g_tx == nullptr || g_server == nullptr || g_conn == BLE_HS_CONN_HANDLE_NONE) return;
  // MTU pro Verbindung abfragen: ein neuer Client kann eine kleinere MTU haben.
  const uint16_t mtu = g_server->getPeerMTU(g_conn);
  const size_t frame = mtu > 3 ? mtu - 3 : 20;
  for (const auto& f : since::splitFrames(msg, frame)) {
    g_tx->setValue(reinterpret_cast<const uint8_t*>(f.data()), f.size());
    bool sent = false;
    for (int attempt = 0; attempt < 5 && !sent; ++attempt) {
      sent = g_tx->notify();
      if (!sent) delay(20);  // Puffer des Controllers voll
    }
    if (!sent) return;  // Rest verwerfen; die App läuft in ihr Timeout und fragt neu
    delay(5);
  }
}

}  // namespace

bool start() {
  auto& st = device::setup();
  NimBLEDevice::init(st.name);
  NimBLEDevice::setMTU(247);
  NimBLEDevice::setPower(3);  // dBm, Setup ist in Reichweite
  NimBLEDevice::setSecurityAuth(/*bonding=*/true, /*mitm=*/true, /*sc=*/true);
  NimBLEDevice::setSecurityIOCap(BLE_HS_IO_DISPLAY_ONLY);
  NimBLEDevice::setSecurityPasskey(st.passkeyNumeric);

  g_server = NimBLEDevice::createServer();
  g_server->setCallbacks(&g_serverCb);
  NimBLEService* svc = g_server->createService(kServiceUuid);
  NimBLECharacteristic* rx = svc->createCharacteristic(
      kRxUuid, NIMBLE_PROPERTY::WRITE | NIMBLE_PROPERTY::WRITE_ENC | NIMBLE_PROPERTY::WRITE_AUTHEN);
  rx->setCallbacks(&g_rxCb);
  g_tx = svc->createCharacteristic(
      kTxUuid, NIMBLE_PROPERTY::NOTIFY | NIMBLE_PROPERTY::READ_ENC | NIMBLE_PROPERTY::READ_AUTHEN);

  NimBLEAdvertising* adv = NimBLEDevice::getAdvertising();
  adv->setName(st.name);
  adv->addServiceUUID(kServiceUuid);
  adv->enableScanResponse(true);
  return adv->start();
}

void loop() {
  std::string req;
  {
    std::lock_guard<std::mutex> lock(g_mutex);
    if (g_pending.empty()) return;
    req = g_pending.front();
    g_pending.pop_front();
  }
  if (req == "\x01too_large") {
    send("{\"id\":null,\"ok\":false,\"error\":\"too_large\"}");
    return;
  }
  send(since::handleRpc(device::hooks(), req));
}

void stop() {
  if (g_server == nullptr) return;
  NimBLEDevice::getAdvertising()->stop();
  for (uint16_t h : g_server->getPeerDevices()) g_server->disconnect(h);
  delay(100);  // Trennung an die Gegenstelle senden
  NimBLEDevice::deinit(true);
  g_server = nullptr;
  g_tx = nullptr;
}

void clearBonds() {
  NimBLEDevice::init("");
  NimBLEDevice::deleteAllBonds();
  NimBLEDevice::deinit(true);
}

}  // namespace ble

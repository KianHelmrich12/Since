// SPDX-License-Identifier: GPL-3.0-or-later
// BLE-GATT-Service nach docs/ble-protocol.md (NimBLE). Nur im Setup-Modus aktiv.
#pragma once

namespace ble {

bool start();  // Name + Passkey aus device::setup()
void loop();   // fertige Anfragen abarbeiten (außerhalb der BLE-Callbacks)
void stop();
void clearBonds();  // Werksreset: gespeicherte Kopplungen löschen

}  // namespace ble

// SPDX-License-Identifier: GPL-3.0-or-later
#include <Arduino.h>

#include "config_store.h"
#include "since_core.h"
#include "version.h"

void setup() {
  Serial.begin(115200);
  // USB-CDC: kurz auf einen Host warten, aber nicht blockieren (Akkubetrieb).
  const unsigned long start = millis();
  while (!Serial && millis() - start < 2000) {
    delay(10);
  }
  Serial.printf("since firmware %s (core %s)\n", SINCE_FW_VERSION, since::coreVersion());
  bool loaded = false;
  const since::Config cfg = since::loadConfig(&loaded);
  Serial.printf("config %s: %s\n", loaded ? "loaded" : "defaults",
                since::toJson(cfg, /*includeSecrets=*/false).c_str());
}

void loop() {
  Serial.println("hello from since");
  delay(5000);
}

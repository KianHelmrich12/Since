# since – Phasenplan

Jede Phase beginnt erst, wenn die Akzeptanzkriterien der vorherigen erfüllt sind.
Schritte mit echter Hardware sind als **MANUELL PRÜFEN** markiert.

## Phase 0 – Repo-Setup
- [x] Struktur wie in CLAUDE.md, .gitignore, .editorconfig, README-Gerüst
- [x] Lizenzdateien: GPL-3.0-or-later (Root), CERN-OHL-S-2.0 (hardware/), CC BY-SA 4.0 (enclosure/)
- [x] platformio.ini mit ee04 (XIAO ESP32-S3, Arduino) und native (Unity)
- [x] Hello-World mit serieller Ausgabe, ein Dummy-Test

Akzeptanz: `pio run -e ee04` und `pio test -e native` laufen grün.

## Phase 1 – Zeitrechnung (since_core)
- [x] Kalenderdifferenz Ereignis → jetzt: Jahre, Monate, Tage, Stunden, Minuten + Gesamttage/-wochen
- [x] Randfälle: Monatsende (31.01. + 1 Monat), Geburt am 29.02., Sommer-/Winterzeit-Wechsel, Ereignis in der Zukunft (Countdown)
- [x] Deutsche Formatierung mit Singular/Plural („1 Jahr“, „2 Jahre“), Nullwerte ausblenden
- [x] Meilensteine erkennen: Geburtstag, 100./1000. Tag, Monatstag

Akzeptanz: mindestens 30 Unit-Tests grün, inkl. aller Randfälle.

## Phase 2 – Konfiguration
- [x] Config: Name, Anlass-Text, Ereignis (lokales Datum + Uhrzeit), TZ, Layout, Update-Intervall, optional WLAN, NTP an/aus, Schema-Version
- [x] JSON (de)serialisieren + validieren in since_core, Persistenz via Preferences in src/
- [x] Sinnvolle Defaults und Migration bei neuer Schema-Version
- [x] JSON-Fixtures (gültig/ungültig, alte Schema-Versionen) in `shared/fixtures/`, die später auch die App für Kontrakttests nutzt

Akzeptanz: Tests für Validierung und Round-Trip grün.

## Phase 3 – Hardware-Bring-up
- [x] board_ee04.h: Display-SPI, Taster, Akku-ADC, I²C für RTC, RTC-fähiger GPIO für DS3231-INT (mit Quellenangabe)
- [x] Display „Hallo since“, DS3231 lesen/stellen, Akkuspannung messen (Code fertig; statt Testbild zeigt die Firmware direkt den Welcome-Screen)
- [ ] Verdrahtung DS3231 laut hardware/wiring.md aufbauen

Akzeptanz: **MANUELL PRÜFEN (offen)** – Text sichtbar, RTC läuft nach Stromtrennung weiter, Spannung plausibel.

## Phase 4 – Darstellung
- [x] Layouts: Standard (Name groß, „3 Jahre · 2 Monate · 14 Tage“, „seit 12.03.2026, 08:15“, Gesamttage), Minimal, Geburtstag, Akku leer, Setup-Modus
- [x] Fonts mit Umlauten, Akku-Symbol
- [x] Optional: Host-Renderer, der Layouts als PNG nach docs/previews/ exportiert (`python3 tools/preview.py`, Fonts angenähert)

Akzeptanz: **MANUELL PRÜFEN (offen)** – alle Screens auf dem Gerät ok.

## Phase 5 – Strom & Deep Sleep
- [x] Ablauf: Aufwachen → Grund prüfen (RTC-Alarm/Timer oder Taster) → RTC lesen → nur bei Änderung neu zeichnen → Display schlafen legen → nächsten Alarm setzen → Deep Sleep
- [x] DS3231-Alarm über INT als ext0/ext1-Weckquelle, ESP-Timer als Fallback
- [x] Akkuschutz: Warnung bei niedrigem Stand, unter Grenzwert „Bitte laden“ anzeigen und lange schlafen

Akzeptanz: **MANUELL PRÜFEN (offen)** – Ruhestrom gemessen und in docs/power.md dokumentiert (Ziel: < 50 µA gesamt).

## Phase 6 – Webportal
- [x] Taster → Hotspot since-XXXX (MAC-Suffix), optional WPA2-Passwort, Captive Portal
- [x] Display zeigt im Setup SSID + QR-Code zum Verbinden
- [x] Seiten: Einstellungen, Uhrzeit vom Handy übernehmen (Browser-Zeit per JS, kein Internet nötig), WLAN für NTP, Status (Akku, FW-Version, letzte Sync), Firmware-Update (OTA-Upload), Werksreset
- [x] API: GET/POST /api/config, POST /api/time, GET /api/status, POST /api/ota
- [x] Nach 10 min Inaktivität: Portal beenden → Deep Sleep

Akzeptanz: **MANUELL PRÜFEN (offen)** – komplette Einrichtung per Handy, überlebt Neustart. Portal-UI gegen Mock im Browser getestet (docs/previews/).

## Phase 6b – Android-App (Angular + zenit-ui, Bluetooth LE)
Zusätzlich zum Portal, das als Fallback ohne App bleibt. OTA läuft weiter nur übers Portal.

Firmware:
- [x] BLE-GATT-Service „since“ mit NimBLE-Arduino. Er läuft nur im Setup-Modus (Taster), gleichzeitig mit dem Hotspot, und endet mit demselben 10-min-Timeout.
- [x] Characteristics: `config` (JSON lesen/schreiben, gestückelt, gleiches Schema wie `/api/config`), `time` (UTC-Epoch schreiben), `status` (lesen + notify: Akku, FW-Version, letzte Sync, RTC-Lost-Power), `command` (Neu zeichnen, Setup beenden)
- [x] Sicherheit: LE Secure Connections mit Passkey, den das E-Paper anzeigt. Schreibende Characteristics nur verschlüsselt. Werksreset gibt es nicht über BLE.
- [x] GATT-Protokoll dokumentiert in `docs/ble-protocol.md` (UUIDs, Datenformate, Fehlercodes, Versionierung)

App (`app/`):
- [x] Angular-Projekt + Capacitor (Android), zenit-ui via `npm install zenit-ui@npm:@zenit-hosting/zenit-ui` + `ng generate zenit-ui:ng-add --themes`
- [x] BLE-Schicht über `@capacitor-community/bluetooth-le` hinter einem Interface, dazu ein Mock-Gerät für Browser-Entwicklung und Tests
- [x] Seiten: Gerät suchen/koppeln, Einstellungen (Name, Anlass, Datum/Uhrzeit, TZ, Layout, Intervall, WLAN/NTP), Uhrzeit vom Handy übernehmen, Status
- [x] Datenmodell + Validierung spiegeln since_core. Kontrakttests gegen `shared/fixtures/`
- [x] Unit-Tests (`ng test`) grün, Debug-APK baut (`npx cap sync android` + Gradle `assembleDebug`) – in der CI (GitHub Actions) grün, APK als Artefakt

Akzeptanz: Tests + APK-Build grün. **MANUELL PRÜFEN**: Auf einem Android-Handy koppeln, Config schreiben und Uhrzeit übernehmen; das Display aktualisiert sich und die Werte überleben einen Neustart.

## Phase 7 – Zeit-Sync & Robustheit
- [x] NTP-Sync wöchentlich, falls WLAN konfiguriert (kurz verbinden, dann WLAN aus)
- [x] RTC-„Lost Power“-Flag → Hinweis „Uhrzeit einstellen“
- [x] Watchdog, Brownout-Handling, Werksreset per 10 s langem Tastendruck

## Phase 8 – Gehäuse
- [x] OpenSCAD, parametrisch (Displaygröße, Akku-/Board-Maße, Wandstärke)
- [x] Frontrahmen mit optionalem Namens-Relief, Rückseite mit USB-C- und Taster-Zugang, Aufsteller + Schlüssellochaufhängung
- [x] Export 3MF, Druckhinweise für Bambu (Material, Multicolor, Supports)

Akzeptanz: **MANUELL PRÜFEN (offen)** – Passform mit `bay_test` und echter Platine, unverifizierte Maße in enclosure/README.md nachmessen.

## Phase 9 – Doku & Release
- [x] README mit Vorschaubildern, BOM mit Bezugsquellen, Bauanleitung (docs/assembly.md), Messanleitung (docs/power.md)
- [ ] Echte Fotos und Strommesswerte nachtragen (**MANUELL PRÜFEN**, siehe docs/abnahme.md)
- [x] CHANGELOG, Release-Workflow für firmware.bin, firmware.factory.bin, littlefs.bin
- [ ] Tag v1.0.0, erst nach bestandener Hardware-Abnahme (docs/abnahme.md)
- [x] Optional: Browser-Flasher mit ESP Web Tools (docs/flasher/, wird per Release-Workflow auf GitHub Pages veröffentlicht)
- [x] CI: Host-Tests, Firmware- und LittleFS-Build bei jedem Push

## Offene Entscheidungen
Werden erfragt, sobald sie relevant werden.
- [x] Lizenz: GPL-3.0-or-later (Firmware), CERN-OHL-S-2.0 (Hardware), CC BY-SA 4.0 (Gehäuse)
- [x] Hosting: GitHub öffentlich (KianHelmrich12/Since), Releases + Web-Flasher über GitHub Pages
- [x] Display: 7,5″ (Firmware); Gehäuse parametrisch auch für 4,2″
- [x] Default-Intervall: „auto“ – nur neu zeichnen, wenn sich der Text ändert (Tagesgrenze, Mitternacht, mit Stundenanzeige stündlich); festes Intervall 10–1440 min wählbar
- [x] App: Capacitor-Android-App, BLE zusätzlich zum Portal, Code in `app/`, Umsetzung nach Phase 6
- [ ] v2-Ideen (nicht Teil von v1.0): mehrere Ereignisse (Geschwister), Farb-E-Paper, Home-Assistant-Anbindung

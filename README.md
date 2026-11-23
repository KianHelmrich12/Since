# since

Ein akkubetriebener E-Paper-Zähler, der die Zeit seit einem Ereignis anzeigt, z. B.

> **Mila**
> 3 Jahre · 2 Monate · 14 Tage
> seit 12.03.2023, 08:15

Gedacht als Geschenk zur Geburt eines Kindes, aber genauso für Hochzeit, Jahrestag oder
Firmengründung. Das Gerät läuft monatelang mit einer Akkuladung, braucht beim Empfänger kein WLAN
und wird per Handy über einen eigenen Hotspot eingerichtet.

> **Status:** Firmware, Portal und Tests sind fertig; die Hardware-Abnahme steht noch aus
> (siehe **MANUELL PRÜFEN** in [PLAN.md](PLAN.md)).

## Funktionen
- Zeigt Jahre · Monate · Tage seit (oder bis) einem Ereignis, dazu Gesamttage und -wochen und Meilensteine (Geburtstag, Tag 100/500/1000 …, Monatstage im ersten und zweiten Jahr).
- Deep Sleep zwischen den Aktualisierungen; gezeichnet wird nur, wenn sich der Text ändert.
- Die DS3231-RTC hält die Uhrzeit ohne WLAN. Ein NTP-Abgleich einmal pro Woche ist optional.
- Einrichtung per Handy, mit derselben API auf zwei Wegen:
  - Taste 1 öffnet einen eigenen Hotspot mit QR-Code und Captive Portal (Einstellungen, Uhrzeit vom Handy, Status, Firmware-Update, Werksreset).
  - Parallel geht es per Bluetooth LE mit der Android-App ([docs/ble-protocol.md](docs/ble-protocol.md)).
- Die Kernlogik (`since_core`) läuft ohne Hardware und ist mit Unit-Tests abgedeckt.

## Bedienung
| Taste | Funktion |
|---|---|
| Taste 1 (KEY0) | Einrichtung starten (10 min); **10 s halten** = Werksreset |
| Taste 2 (KEY1) | Display neu zeichnen |
| Taste 3 (KEY2) | Layout Standard ↔ Minimal |

## Hardware
- Seeed XIAO ePaper Display Board EE04 (XIAO ESP32-S3 Plus)
- 7,5″ E-Paper mono, 800×480, 24-Pin
- DS3231-RTC mit LIR2032
- LiPo 1S 3,7 V, ~2000 mAh, JST-PH 2.0

Verdrahtung: [hardware/wiring.md](hardware/wiring.md) · Stückliste: [hardware/BOM.md](hardware/BOM.md) · Pinbelegung mit Quellen: [hardware/ee04-pinout.md](hardware/ee04-pinout.md) · Strom: [docs/power.md](docs/power.md)

## Firmware bauen
Voraussetzung: [PlatformIO Core](https://docs.platformio.org/en/latest/core/installation/index.html)
und für die Host-Tests ein C++-Compiler (gcc oder clang).

```sh
cd firmware
pio run -e ee04                # bauen
pio run -e ee04 -t upload      # flashen (USB-C)
pio run -e ee04 -t uploadfs    # Web-UI nach LittleFS
pio device monitor -b 115200   # serielle Ausgabe
pio test -e native             # Host-Tests ohne Hardware
python3 tools/portal_mock.py   # Portal-UI ohne Gerät: http://localhost:8080/
```

Ohne Toolchain: Nach einem Release lässt sich die Firmware im Browser installieren (Chrome/Edge), über den Web-Flasher auf GitHub Pages ([docs/flasher/](docs/flasher/)).

## Gehäuse
Parametrisches OpenSCAD-Modell für Bambu-Drucker in [enclosure/](enclosure/) (folgt).

## Lizenzen
| Teil | Lizenz |
|---|---|
| Firmware und alles ohne eigene Lizenzdatei | [GPL-3.0-or-later](LICENSE) |
| Hardware-Doku (`hardware/`) | [CERN-OHL-S-2.0](hardware/LICENSE) |
| Gehäuse (`enclosure/`) | [CC BY-SA 4.0](enclosure/LICENSE) |

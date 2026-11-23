# since

Ein akkubetriebener E-Paper-Zähler, der die Zeit seit einem Ereignis anzeigt, z. B.

> **Mila**
> 3 Jahre · 2 Monate · 14 Tage
> seit 12.03.2023, 08:15

Gedacht als Geschenk zur Geburt eines Kindes, aber genauso für Hochzeit, Jahrestag oder
Firmengründung. Das Gerät läuft monatelang mit einer Akkuladung, braucht beim Empfänger kein WLAN
und wird per Handy über einen eigenen Hotspot eingerichtet.

> **Status:** frühe Entwicklung – siehe [PLAN.md](PLAN.md).

## Hardware
- Seeed XIAO ePaper Display Board EE04 (XIAO ESP32-S3 Plus)
- 7,5″ E-Paper mono, 800×480, 24-Pin
- DS3231-RTC mit LIR2032
- LiPo 1S 3,7 V, ~2000 mAh, JST-PH 2.0

Details, Verdrahtung und BOM folgen in [hardware/](hardware/).

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
```

## Gehäuse
Parametrisches OpenSCAD-Modell für Bambu-Drucker in [enclosure/](enclosure/) (folgt).

## Lizenzen
| Teil | Lizenz |
|---|---|
| Firmware und alles ohne eigene Lizenzdatei | [GPL-3.0-or-later](LICENSE) |
| Hardware-Doku (`hardware/`) | [CERN-OHL-S-2.0](hardware/LICENSE) |
| Gehäuse (`enclosure/`) | [CC BY-SA 4.0](enclosure/LICENSE) |

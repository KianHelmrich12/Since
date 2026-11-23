# Verdrahtung

Pins und Quellen stehen in [ee04-pinout.md](ee04-pinout.md), die Firmware-Konstanten in
`firmware/include/board_ee04.h`.

> **MANUELL PRÜFEN:** Die DS3231-Anbindung ist am Gerät noch nicht verifiziert.
> Vor dem Anlöten Polarität und Spannung mit dem Multimeter prüfen.

## Übersicht
```
 EE04 (XIAO ESP32-S3 Plus)                 DS3231 (ZS042-Modul)
 ─────────────────────────                 ────────────────────
 VCC_3V3 (geschaltet, GPIO43) ──────────── VCC
 GND ───────────────────────────────────── GND
 Pad J4 → GPIO41 (SDA) ─────────────────── SDA
 Pad J4 → GPIO42 (SCL) ─────────────────── SCL
 GPIO8 / D9 ────────────────────────────── SQW/INT
                                           (32K bleibt offen)
 7,5″-Panel ── 24-Pin-FPC, Jumper J5 auf „24 Pin“
 LiPo 1S ───── JST-PH 2.0 (Polarität prüfen!)
```

## Warum so?
- **Standard-I²C-Pins belegt:** D4/D5 sind auf dem EE04 mit KEY2 und ADC_EN belegt. Die freien Pads J4 (unbestückter NFC-Anschluss) führen GPIO41/42 heraus.
  Auf jeder Leitung sitzt ein 300-pF-Kondensator gegen GND (C64/C65), deshalb läuft I²C mit 100 kHz. Die 4,7-kΩ-Pull-ups des ZS042 reichen dafür knapp. Wenn die Kommunikation hakt, C64/C65 auslöten oder zusätzlich 4,7 kΩ nach VCC_3V3 setzen.
- **INT an GPIO8:** GPIO8 ist der einzige freie RTC-fähige Pin, kann also aus dem Deep Sleep wecken. INT ist Open-Drain und aktiv LOW. Die Firmware schaltet im Schlaf den internen RTC-Pull-up ein.
- **VCC an der geschalteten 3,3-V-Leitung:** Die DS3231 zieht an VCC bis ~110 µA (Standby, Datenblatt). Das allein würde das 50-µA-Ziel sprengen.
  Deshalb hängt sie an VCC_3V3. Diese Leitung schaltet die Firmware nur im Wachzustand ein (Lastschalter U16 über GPIO43).
  Im Schlaf läuft die DS3231 auf der LIR2032 weiter. Die Firmware setzt das Bit **BBSQW**, damit der Alarm-Interrupt auch im Batteriebetrieb ausgelöst wird.

## Umbauten am ZS042-Modul
1. **Power-LED entfernen** (oder ihren Vorwiderstand), sonst fließen dauerhaft ~1 mA.
2. **Pull-up an SQW/INT:** Das Widerstandsnetz des ZS042 zieht SQW gegen das Modul-VCC. Im Schlaf ist VCC aus, und SQW würde über den Widerstand auf LOW gezogen. Das löst einen Dauer-Weckgrund aus.
   Abhilfe: den SQW-Pull-up auf dem Modul entfernen (das ist der Widerstand des Netzes, der an SQW geht) und INT nur über den internen Pull-up bzw. extern mit 100 kΩ an **XIAO_3V3** hochziehen.
   **MANUELL PRÜFEN:** Im Schlaf an GPIO8 messen, ob ≈ 3,3 V anliegen.
3. **Ladeschaltung:** Das ZS042 lädt die Knopfzelle über Diode und 200 Ω aus VCC. Das ist für die **LIR2032** gedacht. Mit einer nicht wiederaufladbaren CR2032 die Diode entfernen.
   Bei 3,3 V lädt die Schaltung die LIR2032 nur bis ca. 3,0 V. Für den Gangreserve-Betrieb reicht das.

## Taster
| Taster | Funktion |
|---|---|
| KEY0 (GPIO2) | kurz: Setup-Modus (Hotspot + BLE, 10 min); **10 s halten:** Werksreset |
| KEY1 (GPIO3) | Display sofort neu zeichnen |
| KEY2 (GPIO5) | Layout Standard ↔ Minimal |
| RESET | Neustart |

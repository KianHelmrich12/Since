# Bauanleitung

Dauer: etwa 2–3 Stunden plus Druckzeit. Werkzeug: Lötkolben, Multimeter, Seitenschneider,
Schraubendreher, optional ein Lötkolben-Aufsatz für Gewindeeinsätze.

## 1. Teile besorgen
Siehe [hardware/BOM.md](../hardware/BOM.md).

## 2. Gehäuse drucken
Siehe [enclosure/README.md](../enclosure/README.md). Tipp: Zuerst das kleine Testteil `bay_test`
drucken und an die echte EE04-Platine halten (USB-C, Taster, Schalter). Weicht etwas ab, die als
UNVERIFIED markierten Parameter anpassen und neu exportieren (`enclosure/export.sh`).

## 3. DS3231 vorbereiten und anschließen
Siehe [hardware/wiring.md](../hardware/wiring.md):
1. Power-LED am ZS042 entfernen.
2. Den Pull-up an SQW entfernen (wichtig für den Deep Sleep).
3. Vier Litzen anlöten: VCC → VCC_3V3, GND, SDA → J4/GPIO41, SCL → J4/GPIO42.
4. SQW/INT → D9/GPIO8.
5. LIR2032 einsetzen.

## 4. Display und Akku
1. Jumper J5 auf **„24 Pin“** stellen.
2. FPC des Panels in den 24-Pin-Stecker schieben (Kontakte nach unten, Verriegelung schließen).
3. **Polarität des Akkus prüfen** (Rot = +, an der Platinen-Beschriftung), dann einstecken.

## 5. Firmware installieren
Entweder im Browser über den Web-Flasher (Chrome/Edge, nach einem Release) oder mit PlatformIO:
```sh
cd firmware
pio run -e ee04 -t upload
pio run -e ee04 -t uploadfs
pio device monitor -b 115200
```
Nach dem Start erscheint „Willkommen!“ auf dem Display.

## 6. Einrichten
Taste 1 (KEY0) drücken. Das Display zeigt WLAN-Name, Passwort, einen QR-Code und einen
App-Passkey.
- **Per Browser:** mit dem WLAN verbinden. Das Portal öffnet sich (sonst <http://192.168.4.1>
  aufrufen). Dort Name, Datum und Uhrzeit eintragen und „Uhrzeit vom Handy übernehmen“ drücken.
  Mit „Fertig“ beenden.
- **Per App:** In der since-App „Gerät suchen“ wählen und den Passkey vom Display eingeben.

## 7. Zusammenbauen
Platine auf die Stützen des Rückdeckels setzen, Akku und RTC in ihre Taschen legen, das FPC
einstecken, den Deckel aufsetzen und verschrauben. Den Aufsteller einstecken oder das Gerät an
den Schlüssellöchern aufhängen.

# Hardware-Abnahme (MANUELL PRÜFEN)

Diese Punkte lassen sich nicht ohne Gerät prüfen. Bitte abhaken und die Ergebnisse (Fotos,
Messwerte, serielle Ausgabe) zurückmelden. Erst danach wird `v1.0.0` getaggt.

## Phase 3 – Bring-up
- [ ] Verdrahtung nach [hardware/wiring.md](../hardware/wiring.md) aufgebaut, Umbauten am ZS042 erledigt
- [ ] Nach dem Flashen zeigt das Display „Willkommen!“. Serielle Ausgabe: `since 0.0.1, wake=0`
- [ ] Panel-Controller passt (UC8179 / GxEPD2_750_GDEY075T7). Falls das Bild gestört ist: Aufdruck des Panels melden
- [ ] RTC: Uhrzeit im Portal stellen, USB und Akku 1 min trennen, wieder anschließen → Status zeigt die richtige Uhrzeit, kein „Uhrzeit einstellen“
- [ ] Akkuspannung im Portal (Status) mit dem Multimeter vergleichen (Abweichung < 50 mV, sonst Faktor kalibrieren)

## Phase 4 – Darstellung
- [ ] Standard-, Minimal- (Taste 3), Setup-, Meilenstein- und „Bitte laden“-Screen sehen aus wie in [previews/](previews/). Umlaute und „·“ werden korrekt dargestellt

## Phase 5 – Strom
- [ ] Ruhestrom nach [power.md](power.md) gemessen und eingetragen (Ziel < 50 µA)
- [ ] Im Schlaf liegen an GPIO8 (INT) ≈ 3,3 V an, und das Gerät wacht nicht dauernd auf
- [ ] Gerät wacht zur erwarteten Zeit auf (serielle Ausgabe „sleep until …“) und zeichnet nur bei Änderung neu

## Phase 6 / 6b – Einrichtung
- [ ] Taste 1 → Hotspot erscheint, QR-Code verbindet das Handy, Portal öffnet sich automatisch
- [ ] Einstellungen speichern, „Uhrzeit vom Handy übernehmen“, „Fertig“ → Display zeigt die Daten
- [ ] Nach einem Neustart (RESET) sind die Einstellungen noch da
- [ ] Firmware-Update über „Mehr“ funktioniert
- [ ] App: Gerät finden, mit dem Passkey koppeln, Einstellungen und Uhrzeit setzen, Setup beenden

## Phase 7 – Robustheit
- [ ] Mit WLAN und NTP: serielle Ausgabe zeigt den Sync, Status zeigt „Letzter Zeit-Sync“
- [ ] Taste 1 zehn Sekunden halten → Werksreset, Display zeigt „Willkommen!“

## Phase 8 – Gehäuse
- [ ] `bay_test` passt an die Platine (USB-C, Taster, Schalter)
- [ ] Panel sitzt im Frontrahmen, Fenster deckt den aktiven Bereich ab
- [ ] Gerät steht stabil auf dem Aufsteller, Wandaufhängung hält

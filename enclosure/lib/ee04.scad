// SPDX-License-Identifier: CC-BY-SA-4.0
//
// Seeed "XIAO ePaper Display Board EE04" (mit XIAO ESP32-S3 Plus) – Maße.
//
// Quellen:
//  [DS]  Seeed "Industrial Product Datasheet – XIAO ePaper Display Board
//        (ESP32-S3) – EE04", SKU 100075670,
//        https://files.seeedstudio.com/Bazaar/product_pdf/100075670.pdf
//        S. 5, Maßbild: 81 mm × 44 mm × 7 mm (Länge × Breite × Höhe).
//        Die 44 mm enthalten offenbar die über die Kante ragenden Betätiger
//        (Schalter/Taster/USB-C); die Leiterplatte selbst misst im
//        Draufsichtfoto ≈ 41.5 mm.
//  [HW]  Ebd. S. 5 "Hardware Overview" (Draufsichtfoto, 1920 px breit).
//        Alle Positionen unten wurden aus diesem Foto skaliert
//        (81 mm Leiterplattenlänge = 801 px → 0.101 mm/px), Genauigkeit
//        etwa ±0.5…1 mm.  ⇒ UNVERIFIED – measure.
//  [SCH] Schaltplan V1.2: SW1 = Ein/Aus-Schiebeschalter am Akku, SW2…SW5 =
//        Reset + 3 User-Taster (KEY), Akku-Stecker JST 2.0 (PH).
//        https://files.seeedstudio.com/wiki/Epaper/EE04/XIAO_ePaper_Display_Board_EE04_SCH_V1.2.pdf
//  Das Seeed-Wiki https://wiki.seeedstudio.com/epaper_ee04/ war bei der
//  Recherche nicht erreichbar.
//
// Platinen-Koordinaten (u, v) wie im Draufsichtfoto: Bauteilseite oben,
// Ursprung = linke obere Ecke, u nach rechts, v nach unten. Die "obere" Kante
// trägt Schiebeschalter, Reset, KEY3/KEY2/KEY1 (Silkscreen) und den USB-C des
// XIAO; die untere Kante den 50- und 24-Pin-FPC-Stecker.
// Die Bauteilseite zeigt im Gehäuse zum Rückdeckel, d. h. in der Rückansicht
// sieht man die Platine genau wie auf dem Foto.

ee_size = [81.0, 41.5];   // Leiterplatte u × v      (u: [DS], v: [HW], UNVERIFIED)
ee_pcb_t = 1.6;           // Leiterplattendicke      (UNVERIFIED – Standardwert)
ee_comp_h = 5.4;          // höchstes Bauteil über Leiterplatte (7 mm [DS] − 1.6)

// Befestigungslöcher, Ø ca. 2.2–2.5 mm (aus [HW], UNVERIFIED – measure)
ee_holes = [[2.5, 2.25], [78.5, 2.25], [2.5, 39.25], [78.5, 39.25]];
ee_hole_d = 2.2;

// Bedienelemente an der oberen Kante (v = 0). u = Mitte, prot = Überstand
// über die Kante, z = Höhe der Betätigermitte über der Leiterplatte.
ee_switch = [10.7, 2.1, 1.5];   // Schiebeschalter Akku EIN/AUS (Hebel bewegt sich in u)
ee_reset = [20.5, 1.2, 1.75];
ee_keys = [29.8, 39.3, 48.9];   // KEY3, KEY2, KEY1 (Silkscreen) = 3 User-Taster
ee_key_prot = 1.2;
ee_key_z = 1.75;
ee_usb = [66.1, 2.1, 2.6];      // USB-C am XIAO: Mitte u, Überstand, z-Mitte
ee_usb_size = [9.0, 3.3];       // Buchse b × h

// Weitere Bauteile (nur für Dummy/Kollisionsprüfung)
ee_jst = [70.0, 78.0, 23.5, 32.5];  // JST-PH-Buchse Akku [u0,u1,v0,v1], Stecker kommt von +u
ee_fpc24 = [42.6, 58.4];            // 24-Pin-FPC-Stecker u0..u1 an der Unterkante
ee_fpc24_c = 50.5;
ee_xiao = [57.4, 75.6, -2.1, 21.2]; // XIAO-Modul inkl. USB-Überstand

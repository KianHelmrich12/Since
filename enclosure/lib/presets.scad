// SPDX-License-Identifier: CC-BY-SA-4.0
//
// Display-Presets: Panelmaße und Innenaufteilung.
//
// Alle Koordinaten in der Rückansicht (x rechts, y oben, Ursprung = Panelmitte).
//
// ---------------------------------------------------------------------------
// 7,5" mono 800×480, 24-Pin-FPC, UC8179 (Seeed "7.5 inch Monochrome ePaper",
// Panel-Hersteller Jiangxi Xingtai / SEEKINK, Modell T075A04, bauähnlich zu
// Good Display GDEY075T7).
// Quelle: Datenblatt "Specification for 7.5'' EPD, Model T075A04, Ver. A0,
//   2025-07-29", Kap. 4 und 5 (Mechanical Drawing), verlinkt von Seeed unter
//   https://files.seeedstudio.com/wiki/Other_Display/750-epaper/7.5_inch_Monochrome_ePaper_Display_Datasheet.pdf
//   - Außenmaß (TFT OD) 170.20 ± 0.2 × 111.20 ± 0.2 mm, Dicke 1.10 ± 0.15 mm
//     (ohne Schutzfolie)
//   - Aktive Fläche (TFT AA) 163.20 × 97.92 mm
//   - Rand AA zu Außenkante: 3.49 (Oberkante, gegenüber FPC), 3.50 (links/rechts);
//     unten (FPC-Seite) folgt 111.20 − 97.92 − 3.49 = 9.79 mm
//   - FPC an der langen Unterkante, 12.5 mm breit; Abstand Kante FPC zu
//     Panel-Schmalseite 79.90 ± 0.3 → FPC-Mitte 1.05 mm außermittig
//     (von vorne nach links, in der Rückansicht nach rechts – Richtung
//     aus der gedrehten Zeichnung abgeleitet, ±1 mm, MANUELL PRÜFEN)
//   - FPC-Länge ab Glaskante 24.00 ± 0.3 mm, Versteifung 4.5 mm;
//     Biegebereich steht beim Umlegen ≈ 3 mm über die Glaskante.
//   Good Display nennt für GDEY075T7 dasselbe Außenmaß (170.2 × 111.2) mit
//   1.25 mm Dicke – daher Dicke + Klebeband großzügig.
//
// 4,2" mono 400×300 (Seeed 4.2" Monochrome ePaper / GDEY042T81-Klasse)
//   Außenmaß 91 × 77 × 1.2 mm laut Seeed-Produktseite
//   https://www.seeedstudio.com/4-2-Monochrome-ePaper-Display-with-400x300-Pixels-p-5784.html
//   Aktive Fläche 84.8 × 63.6 mm (GDEY042T81-Klasse).
//   UNVERIFIED – measure: Lage der aktiven Fläche (Ränder), FPC-Lage und -Länge.
// ---------------------------------------------------------------------------

// Indizes der Panel-Datensätze
P_W = 0;        // Außenbreite
P_H = 1;        // Außenhöhe
P_T = 2;        // Dicke
P_AA_W = 3;     // aktive Breite
P_AA_H = 4;     // aktive Höhe
P_AA_SIDE = 5;  // Rand links/rechts
P_AA_TOP = 6;   // Rand oben
P_FPC_X = 7;    // FPC-Mitte (Rückansicht)
P_FPC_BEND_W = 8; // Breite des Biegebereichs
P_FPC_REACH = 9;  // Abstand Panel-Unterkante → Platinen-Unterkante (FPC-Reichweite)

function panel_data(d) =
    d == "4.2"
    ? [91.0, 77.0, 1.2, 84.8, 63.6, 3.1, 3.1, 0.0, 16.0, 11.0]   // UNVERIFIED – measure
    : [170.2, 111.2, 1.1, 163.2, 97.92, 3.50, 3.49, 1.05, 18.0, 13.0];

// Innenaufteilung je Preset:
//   inner     Innenraum [x0, x1, y0, y1]; für 7,5" = Panel + Spiel
//   batt_c    Mittelpunkt Akku, batt_rot = Akku um 90° gedreht
//   rtc_c     Mittelpunkt RTC, rtc_rot
//   socket_c  Mittelpunkt der Aufsteller-Buchse
//   keyholes  Schlüssellöcher (unteres Ende = Einhängeloch)
//   fingers   Niederhalter für das Panel [x, y, vertikal?]
function layout(d) =
    d == "4.2"
    ? [
        [-64, 64, -45, 62],            // inner
        [-10, -10], true,              // Akku quer über der Platine
        [50, -10], false,              // RTC rechts
        [0, 56],                       // Buchse Aufsteller
        [[-56, 46], [56, 46]],         // Schlüssellöcher
        [[-43.5, -30, true], [43.5, 30, true]]
      ]
    : [
        undef,                         // inner = Panel + Spiel (s. derived.scad)
        [57.5, -4], false,             // Akku rechts neben der Platine
        [-68, -20], false,             // RTC links neben der Platine
        [0, 44],
        [[-55, 38], [55, 38]],
        [[-35, 53.4, false], [35, 53.4, false],
         [-60, -53.4, false], [60, -53.4, false],
         [-83.1, 20, true], [83.1, 40, true]]
      ];

L_INNER = 0; L_BATT_C = 1; L_BATT_ROT = 2; L_RTC_C = 3; L_RTC_ROT = 4;
L_SOCKET = 5; L_KEYHOLES = 6; L_FINGERS = 7;

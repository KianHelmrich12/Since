// SPDX-License-Identifier: CC-BY-SA-4.0
//
// since – parametrisches Gehäuse für den E-Paper-Zähler
//
// Hauptdatei. Alle einstellbaren Parameter stehen hier (OpenSCAD-Customizer),
// die Geometrie liegt in lib/*.scad.
//
// Export einzelner Teile (siehe README.md):
//   openscad -D 'part="front"' -o exports/since-7.5-front.stl since.scad
//
// Koordinatensystem der Baugruppe (alle Teile werden darin konstruiert):
//   z = 0     Sichtseite (Vorderseite) des Frontrahmens, z wächst nach hinten
//   x, y      so, wie man von HINTEN auf das Gerät schaut (x rechts, y oben).
//             Von vorne gesehen ist x also gespiegelt.
//   Ursprung  Mitte des Panels.

/* [Teil / Ansicht] */
// Welches Teil gerendert/exportiert wird
part = "all"; // [all, front, back, stand, text, standing]
// Displaygröße (Preset)
display = "7.5"; // [7.5, 4.2]
// Explosionsabstand in der Ansicht "all" (mm)
explode = 0; // [0:5:60]
// Dummy-Hardware (Panel, Platine, Akku, RTC) in "all" einblenden
show_hardware = true;

/* [Name / Relief] */
// Text auf dem Kinn unter dem Display ("" = kein Text)
name_text = "since";
// Schrift; muss Umlaute enthalten
name_font = "Liberation Sans:style=Bold";
// Schriftgröße (≈ Versalhöhe in mm)
name_size = 8; // [4:0.5:14]
// inlay = bündige Einlage (AMS), engraved = vertieft, raised = erhaben
name_style = "inlay"; // [inlay, engraved, raised, none]
// Tiefe bzw. Höhe des Reliefs (mm), Vielfaches der Schichthöhe
name_depth = 0.6; // [0.2:0.2:1.2]
// Verschiebung des Textes nach oben/unten (mm, + = Richtung Display)
name_y_offset = 0;
// Zeichenabstand
name_spacing = 1.0;

/* [Gehäuse] */
// Rahmenbreite links/rechts/oben um den Innenraum (mm)
rim = 5.5;
// Rahmenbreite unten (Kinn für den Namen) (mm)
chin = 11;
// Stärke der Frontplatte vor dem Panel (mm)
front_t = 2.0;
// Stärke des Rückdeckels (mm)
back_t = 2.0;
// Eckenradius außen (mm)
corner_r = 4;
// Fase an der Vorderkante (mm)
front_chamfer = 1.0;
// Spiel um das Panel je Seite (mm)
panel_tol = 0.3;
// Doppelseitiges Klebeband zwischen Panel und Rahmen (mm)
tape_t = 0.2;
// Fenster ist je Seite so viel größer als die aktive Fläche (mm, negativ = Rand abdecken)
window_reveal = 0.5;
// Mindestwandstärke innerer Wände (mm)
wall = 1.6;
// Spiel für Passungen (Deckel, Aufsteller) (mm)
fit_tol = 0.3;

/* [Schrauben Deckel] */
// selftap = M2-Kunststoffschraube direkt ins Loch, insert = M2-Gewindeeinsatz
screw_type = "selftap"; // [selftap, insert]
// Kernloch für selbstschneidende Schraube (mm)
selftap_hole = 1.7;
// Bohrung für Gewindeeinsatz M2 (mm)
insert_hole = 3.2;
// Länge des Gewindeeinsatzes (mm)
insert_len = 4.0;
// Durchgangsloch im Deckel (mm)
screw_clear = 2.4;
// Senkkopfdurchmesser (mm)
screw_head_d = 4.2;
// Lochtiefe im Rahmen (mm)
screw_depth = 9;

/* [Platine EE04] */
// Abstand Panel-Rückseite → Platinenunterseite (Schaumstoff-Pads) (mm)
board_gap = 2.5;
// Zusätzliche Verschiebung der Platine [x, y] (mm)
board_shift = [0, 0];
// Freiraum USB-C-Stecker (Umspritzung) Breite × Höhe (mm)
usb_plug = [12.5, 7.0];
// Länge der Service-Mulde (Platz für den USB-C-Stecker) (mm)
bay_len = 28;
// Taster: flexure = gedruckte Biegezungen, hole = Löcher für einen Stift
button_style = "flexure"; // [flexure, hole]
// Länge der Biegezunge (mm)
flex_len = 10;
// Stärke der Biegezunge (mm)
flex_t = 1.2;

/* [Akku] */
// LiPo-Maße [Breite, Länge, Dicke] (mm)
battery = [50, 60, 8];
// Spiel um den Akku je Seite (mm)
battery_margin = 1.0;

/* [RTC DS3231 ZS042] */
// Modulmaße [Breite, Länge, Höhe inkl. Knopfzelle] (mm)
rtc = [22, 38, 14];
// Spiel je Seite (mm)
rtc_margin = 1.0;

/* [Aufsteller] */
// Neigung nach hinten (Grad)
stand_angle = 18; // [10:1:30]
// Breite des Aufstellers (mm)
stand_w = 30;
// Dicke der Steckzunge (mm)
stand_t = 5;
// Einstecktiefe der Zunge (mm)
stand_tongue = 12;

/* [Wandaufhängung] */
// Schlüssellöcher im Deckel
keyhole = true;
// Kopfdurchmesser der Wandschraube + Spiel (mm)
keyhole_head_d = 8.5;
// Schaftdurchmesser + Spiel (mm)
keyhole_shank_d = 4.2;
// Schiebeweg (mm)
keyhole_travel = 8;
// Tiefe der Kopftasche (mm)
keyhole_pocket = 3.0;

/* [Hidden] */
$fn = 48;
eps = 0.01;

include <lib/util.scad>
include <lib/presets.scad>
include <lib/ee04.scad>
include <lib/derived.scad>
include <lib/hardware.scad>
include <lib/front.scad>
include <lib/back.scad>
include <lib/stand.scad>

report();

if (part == "front") {
    front_frame();
} else if (part == "text") {
    name_inlay();
} else if (part == "back") {
    // Druckorientierung: Außenseite des Deckels auf dem Druckbett
    translate([0, 0, H + back_t]) rotate([180, 0, 0]) back_cover();
} else if (part == "stand") {
    stand_print();
} else if (part == "standing") {
    standing();
} else {
    assembly();
}

module assembly() {
    color("WhiteSmoke") front_frame();
    if (name_style == "inlay" && name_text != "") color("SteelBlue") name_inlay();
    color("Gainsboro") translate([0, 0, 2 * explode]) back_cover();
    color("Peru") translate([0, 0, 3 * explode]) stand_assembled();
    if (show_hardware) translate([0, 0, explode]) hardware_dummies();
}

// Baugruppe auf dem Aufsteller stehend (nur Ansicht): Standkante = hintere Unterkante
module standing() {
    rotate([0, stand_angle, 0])
        translate([-depth, 0, -outer_r[2]])
            multmatrix([[0, 0, 1, 0], [1, 0, 0, 0], [0, 1, 0, 0], [0, 0, 0, 1]])
                assembly();
    // Tischplatte
    color("BurlyWood") translate([-60, -outer_w / 2 - 20, -2]) cube([180, outer_w + 40, 2]);
}

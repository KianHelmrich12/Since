// SPDX-License-Identifier: CC-BY-SA-4.0
//
// Abgeleitete Maße. Wird nach den Parametern in since.scad eingebunden.

PD = panel_data(display);
LY = layout(display);

panel_w = PD[P_W];
panel_h = PD[P_H];
panel_t = PD[P_T];
// Panelaußenkante [x0, x1, y0, y1]
panel_r = [-panel_w / 2, panel_w / 2, -panel_h / 2, panel_h / 2];
// Aktive Fläche
aa_r = [panel_r[0] + PD[P_AA_SIDE], panel_r[1] - PD[P_AA_SIDE],
        panel_r[3] - PD[P_AA_TOP] - PD[P_AA_H], panel_r[3] - PD[P_AA_TOP]];
window_r = rect_grow(aa_r, window_reveal);
fpc_x = PD[P_FPC_X];
fpc_bend_w = PD[P_FPC_BEND_W];

// Innenraum und Außenkontur
pocket_r = rect_grow(panel_r, panel_tol);
inner_r = LY[L_INNER] == undef ? pocket_r : LY[L_INNER];
outer_r = [inner_r[0] - rim, inner_r[1] + rim, inner_r[2] - chin, inner_r[3] + rim];
outer_w = outer_r[1] - outer_r[0];
outer_h = outer_r[3] - outer_r[2];
// Ringwand um das Panel, falls der Innenraum größer als das Panel ist (4,2")
panel_ring = LY[L_INNER] != undef;

// z-Stapel (von vorne nach hinten)
panel_front_z = front_t + tape_t;
panel_back_z = panel_front_z + panel_t;
pcb_bot_z = panel_back_z + board_gap;
pcb_top_z = pcb_bot_z + ee_pcb_t;
comp_top_z = pcb_top_z + ee_comp_h;

// Akku / RTC: Footprints (inkl. Spiel)
batt_rot = LY[L_BATT_ROT];
batt_fp = batt_rot ? [battery[1], battery[0]] : [battery[0], battery[1]];
batt_r = rect_grow(rect_c(LY[L_BATT_C], batt_fp), battery_margin);
rtc_rot = LY[L_RTC_ROT];
rtc_fp = rtc_rot ? [rtc[1], rtc[0]] : [rtc[0], rtc[1]];
rtc_r = rect_grow(rect_c(LY[L_RTC_C], rtc_fp), rtc_margin);

// Lage der Platine: Der 24-Pin-Stecker sitzt über dem FPC, die Unterkante der
// Platine liegt PD[P_FPC_REACH] mm über der Panel-Unterkante (FPC-Länge 24 mm,
// davon ≈ 3 mm Biegung, ≈ 4 mm im Stecker, Rest S-Bogen).
board_x0 = fpc_x - ee_fpc24_c + board_shift[0];
board_top_y = panel_r[2] + PD[P_FPC_REACH] + ee_size[1] + board_shift[1];
// Platinenpunkt (u, v) → Rückansicht (x, y)
function ee_xy(u, v) = [board_x0 + u, board_top_y - v];
board_r = [board_x0, board_x0 + ee_size[0], board_top_y - ee_size[1], board_top_y];

// Innenhöhe H = z der Deckelinnenseite. Der Akku liegt am Deckel und darf über
// Platine/JST-Stecker ragen, die RTC steht am Deckel.
H = max(comp_top_z + battery[2] + 1.0,
        panel_back_z + rtc[2] + 1.0,
        comp_top_z + 2.0);
depth = H + back_t;

// Service-Mulde im Deckel vor der Bedienkante der Platine
bay_gap = 2.6;                              // Platinenkante → Wand (USB ragt 2.1 mm)
bay_wall_y0 = board_top_y + bay_gap;        // platinenseitige Wandfläche
bay_wall_y1 = bay_wall_y0 + wall;
bay_r = [board_x0 + 4, board_x0 + 74, bay_wall_y1, bay_wall_y1 + bay_len];  // Öffnung
usb_z = pcb_top_z + ee_usb[2];
key_z = pcb_top_z + ee_key_z;
switch_z = pcb_top_z + ee_switch[2];
bay_floor_top = usb_z - usb_plug[1] / 2 - 0.3;
bay_floor_bot = bay_floor_top - 1.2;

// Schraubenpositionen im Rahmen (Mitte des Rahmens neben dem Innenraum)
// Oben und seitlich in der Mitte des Rahmens, unten in der Mitte des Kinns;
// unten nicht mittig, dort liegt die FPC-Aussparung.
screw_off = rim / 2;
screw_bot_y = inner_r[2] - chin / 2;
screw_pts = [
    [inner_r[0] - screw_off, screw_bot_y],
    [inner_r[1] + screw_off, screw_bot_y],
    [inner_r[0] - screw_off, inner_r[3] + screw_off],
    [inner_r[1] + screw_off, inner_r[3] + screw_off],
    [0, inner_r[3] + screw_off],
    [-outer_w / 4, screw_bot_y],
    [outer_w / 4, screw_bot_y]
];

// Aufsteller-Buchse
socket_c = LY[L_SOCKET];
socket_hole = [stand_w + 2 * fit_tol, stand_t + 2 * fit_tol];

// Text: mittig zwischen Fensterunterkante und Rahmenunterkante
name_y = (window_r[2] - front_t + outer_r[2]) / 2 + name_y_offset;

module report() {
    echo(str("since enclosure ", display, "\": Außenmaß ", outer_w, " × ", outer_h,
             " × ", depth, " mm, Fenster ", window_r[1] - window_r[0], " × ",
             window_r[3] - window_r[2], " mm"));
    echo(str("z: Panel-Rückseite ", panel_back_z, ", Platine ", pcb_bot_z, "–", pcb_top_z,
             ", Deckel innen H=", H, ", Muldenboden ", bay_floor_bot, "–", bay_floor_top));
    assert(outer_w <= 250 && outer_h <= 250, "Teil passt nicht auf ein 256-mm-Bett");
    assert(bay_floor_bot >= panel_back_z + 0.8, "Muldenboden kollidiert mit dem Panel – board_gap erhöhen");
    assert(rim >= 2 * wall, "rim zu klein");
    assert(screw_type == "selftap" || rim / 2 - insert_hole / 2 >= 1.2,
           "rim zu klein für Gewindeeinsätze (rim >= 6.5 empfohlen)");
}

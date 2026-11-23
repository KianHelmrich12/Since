// SPDX-License-Identifier: CC-BY-SA-4.0
//
// Rückdeckel: Platte mit Zentrierkragen, Niederhaltern für Panel und Platine,
// Service-Mulde (USB-C, Taster, Reset, Ein/Aus), Akku- und RTC-Fach,
// Buchse für den Aufsteller und Schlüssellöcher.
// Modelliert in Einbaulage (Platte bei z = H … H + back_t). Gedruckt wird
// mit der Außenseite nach unten (since.scad dreht das Teil für den Export).

module back_cover() {
    if (button_style == "flexure") flex_nubs();
    difference() {
        union() {
            rbox_plate();
            skirt();
            for (f = LY[L_FINGERS]) panel_finger(f);
            for (h = ee_holes) board_post(ee_xy(h[0], h[1]));
            bay_body();
            battery_fence();
            rtc_fence();
            socket_body();
            if (keyhole) for (k = LY[L_KEYHOLES]) keyhole_body(k);
        }
        for (p = screw_pts) screw_hole_cover(p);
        bay_cuts();
        socket_cut();
        if (keyhole) for (k = LY[L_KEYHOLES]) keyhole_cut(k);
    }
}

module rbox_plate() {
    translate([0, 0, H]) linear_extrude(back_t) rrect2d(outer_r, corner_r);
}

// Zentrierkragen, greift in den Innenraum des Frontrahmens
skirt_h = 3;
module skirt() {
    translate([0, 0, H - skirt_h])
        linear_extrude(skirt_h + eps)
            rect_ring2d(rect_grow(inner_r, -fit_tol - wall), wall);
}

// Niederhalter: drückt über ein Schaumstoffpad (≈ 1 mm) auf den Panelrand
finger_gap = 0.8;
module panel_finger(f) {
    s = f[2] ? [wall, 8] : [8, wall];
    rbox(rect_c([f[0], f[1]], s), panel_back_z + finger_gap, H + eps);
}

// Säule mit Zentrierstift: klemmt die Platine gegen Schaumstoffpads auf dem Panel
module board_post(p) {
    zcyl(p, 4.4, pcb_top_z, H + eps);
    // Stift in das Befestigungsloch (Ø ee_hole_d), angefast
    translate([p[0], p[1], pcb_top_z - 1.4]) {
        cylinder(d1 = 1.2, d2 = ee_hole_d - 0.4, h = 0.6);
        translate([0, 0, 0.6]) cylinder(d = ee_hole_d - 0.4, h = 0.8 + eps);
    }
}

// --- Service-Mulde ---------------------------------------------------------
module bay_body() {
    r_out = [bay_r[0] - wall, bay_r[1] + wall, bay_wall_y0, bay_r[3] + wall];
    rbox(r_out, bay_floor_bot, H + eps);
}

module bay_cuts() {
    // Mulde selbst (offen zur Rückseite)
    rbox(bay_r, bay_floor_top, depth + 1);
    // USB-C: Durchbruch für die Steckerumspritzung
    bay_wall_cut(ee_usb[0], usb_z, usb_plug[0] + 0.5, usb_plug[1] + 0.5, 1.5);
    // Ein/Aus-Schalter: Schlitz, Hebel mit Zahnstocher/Stift schieben
    bay_wall_cut(ee_switch[0], switch_z, 8.0, 3.2, 0.8);
    // Reset: Loch für eine Büroklammer
    translate([board_x0 + ee_reset[0], bay_wall_y0 - 1, key_z])
        rotate([-90, 0, 0]) cylinder(d = 2.0, h = wall + 2);
    // User-Taster
    for (u = ee_keys) {
        if (button_style == "flexure") flex_cut(board_x0 + u);
        else translate([board_x0 + u, bay_wall_y0 - 1, key_z])
            rotate([-90, 0, 0]) cylinder(d = 3.0, h = wall + 2);
    }
}

// rechteckiger Durchbruch durch die platinenseitige Muldenwand
module bay_wall_cut(u, zc, w, h, rr) {
    translate([board_x0 + u, bay_wall_y0 - 1, zc])
        rotate([-90, 0, 0])
            linear_extrude(wall + 2)
                offset(r = rr) square([w - 2 * rr, h - 2 * rr], center = true);
}

// Biegezunge: U-förmiger Schlitz, Gelenk oben (zur Deckelplatte hin),
// Nocken auf der Platinenseite in Höhe des Tasterstößels.
flex_w = 6.0;
flex_slit = 0.6;
module flex_cut(x) {
    z0 = bay_floor_top + flex_slit;          // freies Ende
    z1 = z0 + flex_len;                      // Gelenk
    // Schlitze links/rechts und unten
    for (s = [-1, 1])
        translate([x + s * (flex_w + flex_slit) / 2 - flex_slit / 2, bay_wall_y0 - 1, z0 - flex_slit])
            cube([flex_slit, wall + 2, flex_len + flex_slit]);
    translate([x - flex_w / 2 - flex_slit, bay_wall_y0 - 1, z0 - flex_slit])
        cube([flex_w + 2 * flex_slit, wall + 2, flex_slit]);
    // Zunge von der Muldenseite her auf flex_t ausdünnen
    translate([x - flex_w / 2 - eps, bay_wall_y0 + flex_t, z0 - eps])
        cube([flex_w + 2 * eps, wall, flex_len * 0.8]);
}

// Nocken der Biegezungen (werden nach dem Differenzieren angefügt)
module flex_nubs() {
    p = bay_gap - ee_key_prot - 0.3;         // Überstand zur Platine
    for (u = ee_keys)
        translate([board_x0 + u - 2, bay_wall_y0 + eps, key_z])
            rotate([90, 0, 90])
                linear_extrude(4)
                    polygon([[0, -2.0], [-p, -2.0 + p], [-p, 2.0 - p], [0, 2.0]]);
}

// --- Akku und RTC -------------------------------------------------------------
fence_h = 4;
module battery_fence() {
    difference() {
        translate([0, 0, H - fence_h]) linear_extrude(fence_h + eps) rect_ring2d(batt_r, wall);
        // Kabeldurchlass zur Platine (linke Seite)
        rbox([batt_r[0] - wall - 1, batt_r[0] + 1,
              (batt_r[2] + batt_r[3]) / 2 - 6, (batt_r[2] + batt_r[3]) / 2 + 6], H - fence_h - 1, H);
    }
}

module rtc_fence() {
    // Schmalseiten offen (Stiftleisten des ZS042), nur Ecken als Anschlag
    long_y = rtc_r[3] - rtc_r[2] >= rtc_r[1] - rtc_r[0];
    open = long_y ? (rtc_r[1] - rtc_r[0]) - 6 : (rtc_r[3] - rtc_r[2]) - 6;
    cx = (rtc_r[0] + rtc_r[1]) / 2;
    cy = (rtc_r[2] + rtc_r[3]) / 2;
    difference() {
        translate([0, 0, H - fence_h]) linear_extrude(fence_h + eps) rect_ring2d(rtc_r, wall);
        if (long_y) rbox(rect_c([cx, cy], [open, 200]), H - fence_h - 1, H);
        else rbox(rect_c([cx, cy], [200, open]), H - fence_h - 1, H);
    }
}

// --- Aufsteller-Buchse ----------------------------------------------------
module socket_body() {
    rbox(rect_c(socket_c, socket_hole + [2 * wall, 2 * wall]), H - stand_tongue - 1 - 1.2, H + eps);
}

module socket_cut() {
    rbox(rect_c(socket_c, socket_hole), H - stand_tongue - 1, depth + 1);
}

// --- Schlüssellöcher ------------------------------------------------------
module keyhole_body(k) {
    translate([0, 0, H - keyhole_pocket - 1.2])
        linear_extrude(keyhole_pocket + 1.2 + eps)
            slot2d(k, k + [0, keyhole_travel], keyhole_head_d + 1 + 2 * wall);
}

module keyhole_cut(k) {
    // Kopftasche hinter der Platte
    translate([0, 0, H - keyhole_pocket])
        linear_extrude(keyhole_pocket + eps)
            slot2d(k, k + [0, keyhole_travel], keyhole_head_d + 1);
    // Durchbruch in der Platte: Einhängeloch unten, Schlitz nach oben
    translate([0, 0, H - eps]) linear_extrude(back_t + 1) {
        translate(k) circle(d = keyhole_head_d);
        slot2d(k, k + [0, keyhole_travel], keyhole_shank_d);
    }
}

// --- Deckelschrauben ------------------------------------------------------
module screw_hole_cover(p) {
    zcyl(p, screw_clear, H - 1, depth + 1);
    // Senkung 90°
    // (Kegel beginnt innerhalb der Bohrung, damit keine Kanten zusammenfallen)
    cs_d0 = screw_clear - 0.4;
    translate([p[0], p[1], depth - (screw_head_d - cs_d0) / 2])
        cylinder(d1 = cs_d0, d2 = screw_head_d, h = (screw_head_d - cs_d0) / 2 + eps);
}

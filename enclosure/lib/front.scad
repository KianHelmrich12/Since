// SPDX-License-Identifier: CC-BY-SA-4.0
//
// Frontrahmen: Sichtplatte mit Fenster, Panel-Tasche, Seitenwände in voller
// Gehäusetiefe, Schraublöcher für den Deckel, optionaler Namenszug.
// Druckorientierung: Sichtseite nach unten (so wie modelliert, z = 0 auf dem Bett).

module front_frame() {
    difference() {
        union() {
            front_shell();
            if (name_style == "raised" && name_text != "")
                translate([0, 0, -name_depth]) name_body(name_depth + eps);
        }
        // Innenraum
        rbox(inner_r, front_t, H + 1);
        // Fenster mit 45°-Fase zur Sichtseite (beim Druck mit der Sichtseite
        // nach unten ist das ein 45°-Überhang)
        window_cut();
        // Platz für den Biegebereich des Panel-FPC
        fpc_notch();
        // Deckelschrauben
        for (p = screw_pts) screw_hole_frame(p);
        // Namenszug vertieft bzw. Tasche für die Einlage
        if ((name_style == "inlay" || name_style == "engraved") && name_text != "")
            translate([0, 0, -eps]) name_body(name_depth + eps);
    }
    if (panel_ring) panel_ring_wall();
}

module front_shell() {
    hull() {
        translate([0, 0, front_chamfer])
            linear_extrude(H - front_chamfer) rrect2d(outer_r, corner_r);
        linear_extrude(eps)
            rrect2d(rect_grow(outer_r, -front_chamfer), max(corner_r - front_chamfer, 0.5));
    }
}

module window_cut() {
    hull() {
        rbox(window_r, front_t, front_t + 1);
        rbox(rect_grow(window_r, front_t + 1), -1, -1 + eps);
    }
}

module fpc_notch() {
    notch_w = fpc_bend_w + 4;
    r = [fpc_x - notch_w / 2, fpc_x + notch_w / 2,
         pocket_r[2] - 3.5, pocket_r[2] + 1];
    rbox(r, front_t, H + 1);
}

// Ringwand um das Panel, nur wenn der Innenraum größer als das Panel ist
module panel_ring_wall() {
    difference() {
        translate([0, 0, front_t - eps])
            linear_extrude(panel_back_z - front_t)
                rect_ring2d(pocket_r, wall);
        fpc_notch();
    }
}

module screw_hole_frame(p) {
    if (screw_type == "insert") {
        zcyl(p, insert_hole, H - insert_len - 0.5, H + 1);
        zcyl(p, selftap_hole, H - screw_depth, H + 1);
    } else {
        zcyl(p, selftap_hole, H - screw_depth, H + 1);
    }
    // kleine Fase am Lochanfang
    translate([p[0], p[1], H - 0.5 - 0.07])
        cylinder(d1 = selftap_hole - 0.2, d2 = selftap_hole + 1.2, h = 0.5 + eps);
}

// 2D-Namenszug, so gespiegelt, dass er von vorne lesbar ist
module name_2d() {
    translate([0, name_y])
        mirror([1, 0])
            text(name_text, size = name_size, font = name_font,
                 halign = "center", valign = "center", spacing = name_spacing);
}

module name_body(h) {
    linear_extrude(h) name_2d();
}

// Einlage für den Mehrfarbdruck (gleiche Lage wie im Frontrahmen)
module name_inlay() {
    if (name_text != "") name_body(name_depth);
}

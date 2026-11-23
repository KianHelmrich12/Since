// SPDX-License-Identifier: CC-BY-SA-4.0
//
// Vereinfachte Dummies der Hardware für die Baugruppenansicht und die
// Sichtprüfung auf Kollisionen. Werden nicht exportiert.

module hardware_dummies() {
    // Panel (aktive Fläche dunkler)
    color("DimGray") rbox(panel_r, panel_front_z, panel_back_z);
    color("Ivory") rbox(aa_r, panel_front_z - 0.05, panel_front_z);
    // FPC: um die Unterkante gebogen, auf der Rückseite nach oben zum Stecker
    color("Orange") {
        fpc_r = [fpc_x - 6.25, fpc_x + 6.25];
        rbox([fpc_r[0], fpc_r[1], panel_r[2] - 2.5, board_r[2] + 3.5], panel_back_z, panel_back_z + 0.3);
    }
    ee04_dummy();
    // Akku (am Deckel)
    color("RoyalBlue")
        rbox(rect_grow(batt_r, -battery_margin), H - battery[2], H);
    // RTC (am Deckel)
    color("SeaGreen")
        rbox(rect_grow(rtc_r, -rtc_margin), H - rtc[2], H);
}

module ee04_dummy() {
    // Leiterplatte
    color("Black") rbox(board_r, pcb_bot_z, pcb_top_z);
    // Bauteilhüllkurve (flach) und markante Bauteile
    color("DarkSlateGray") {
        // XIAO inkl. USB-C
        let(a = ee_xy(ee_xiao[0], ee_xiao[3]), b = ee_xy(ee_xiao[1], ee_xiao[2]))
            rbox([a[0], b[0], a[1], b[1]], pcb_top_z, pcb_top_z + 2.2);
        // JST
        let(a = ee_xy(ee_jst[0], ee_jst[3]), b = ee_xy(ee_jst[1], ee_jst[2]))
            rbox([a[0], b[0], a[1], b[1]], pcb_top_z, comp_top_z);
        // FPC-Stecker
        let(a = ee_xy(ee_fpc24[0], ee_size[1]), b = ee_xy(ee_fpc24[1], ee_size[1] - 5))
            rbox([a[0], b[0], a[1], b[1]], pcb_top_z, pcb_top_z + 2.0);
    }
    color("Silver") {
        // USB-C-Buchse
        let(c = ee_xy(ee_usb[0], 0))
            translate([c[0], c[1] + ee_usb[1] - 7.3, usb_z])
                rotate([-90, 0, 0])
                    linear_extrude(7.3)
                        offset(r = 1.3) square([ee_usb_size[0] - 2.6, ee_usb_size[1] - 2.6], center = true);
        // Taster + Reset (seitlich betätigt)
        for (u = concat([ee_reset[0]], ee_keys)) let(c = ee_xy(u, 0)) {
            rbox(rect_c([c[0], c[1] - 2], [6, 4]), pcb_top_z, pcb_top_z + 3.5);
            rbox(rect_c([c[0], c[1] + ee_key_prot / 2], [2.0, ee_key_prot]), key_z - 0.75, key_z + 0.75);
        }
        // Schiebeschalter
        let(c = ee_xy(ee_switch[0], 0)) {
            rbox(rect_c([c[0], c[1] - 2], [9, 4]), pcb_top_z, pcb_top_z + 3.0);
            rbox(rect_c([c[0], c[1] + ee_switch[1] / 2], [1.5, ee_switch[1]]), switch_z - 0.75, switch_z + 0.75);
        }
    }
}

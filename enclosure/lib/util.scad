// SPDX-License-Identifier: CC-BY-SA-4.0
//
// Hilfsmodule ohne Bezug zu Projektparametern.

// Rechteck [x0, x1, y0, y1] als 2D-Form
module rect2d(r) {
    translate([r[0], r[2]]) square([r[1] - r[0], r[3] - r[2]]);
}

// Rechteck mit abgerundeten Ecken (Radius rr)
module rrect2d(r, rr) {
    if (rr <= 0) rect2d(r);
    else offset(r = rr) offset(delta = -rr) rect2d(r);
}

// Quader aus Rechteck [x0, x1, y0, y1] und z-Bereich [z0, z1]
module rbox(r, z0, z1) {
    translate([0, 0, z0]) linear_extrude(z1 - z0) rect2d(r);
}

// Rechteck um Mittelpunkt c mit Größe s
function rect_c(c, s) = [c[0] - s[0] / 2, c[0] + s[0] / 2, c[1] - s[1] / 2, c[1] + s[1] / 2];

// Rechteck vergrößern (d > 0) oder verkleinern
function rect_grow(r, d) = [r[0] - d, r[1] + d, r[2] - d, r[3] + d];

// Langloch zwischen zwei Punkten (2D)
module slot2d(p0, p1, d) {
    hull() {
        translate(p0) circle(d = d);
        translate(p1) circle(d = d);
    }
}

// Rahmen (Ring) aus einem Rechteck, Wandstärke t nach außen
module rect_ring2d(r, t) {
    difference() {
        rect2d(rect_grow(r, t));
        rect2d(r);
    }
}

// Zylinder entlang z von z0 bis z1
module zcyl(p, d, z0, z1) {
    translate([p[0], p[1], z0]) cylinder(d = d, h = z1 - z0);
}

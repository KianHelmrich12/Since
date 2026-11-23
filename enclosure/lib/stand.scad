// SPDX-License-Identifier: CC-BY-SA-4.0
//
// Abnehmbarer Aufsteller: Steckzunge (senkrecht zum Deckel) + Bein.
// Wird als 2D-Profil konstruiert und auf stand_w extrudiert; gedruckt liegt
// das Profil flach auf dem Bett (keine Stützen, Schichten in Lastrichtung).
//
// Profil-Koordinaten (p, q): p = Richtung nach hinten (= z der Baugruppe),
// q = Höhe über der Rahmenunterkante. Das Gerät steht auf der hinteren
// Unterkante (p = depth, q = 0) und ist um stand_angle nach hinten geneigt.

stand_qs = socket_c[1] - outer_r[2];             // Höhe der Buchse über Unterkante
stand_hub = 4;                                    // Bund hinter dem Deckel
stand_pad = [24, 4];                              // Fuß (Länge × Höhe, Weltkoordinaten)

// Welt (x nach hinten, y nach oben, Ursprung Standkante) ← Profil
function st_world(pt) = let(a = stand_angle, d = pt - [depth, 0])
    [d[0] * cos(a) + d[1] * sin(a), -d[0] * sin(a) + d[1] * cos(a)];
// Profil ← Welt
function st_local(w) = let(a = stand_angle)
    [w[0] * cos(a) - w[1] * sin(a) + depth, w[0] * sin(a) + w[1] * cos(a)];

// Fuß: so weit hinter der Buchse, dass das Bein ~25° steil ist
stand_sock_w = st_world([depth, stand_qs]);
stand_foot_x = stand_sock_w[0] + 0.45 * stand_sock_w[1];

module stand_2d() {
    th = stand_t;
    // Steckzunge
    translate([depth - stand_tongue, stand_qs - th / 2])
        square([stand_tongue + eps, th]);
    // Bund + Bein
    hull() {
        translate([depth, stand_qs - th / 2 - 2]) square([stand_hub, th + 4]);
        translate(st_local([stand_foot_x, 3.5])) circle(d = 7);
    }
    // Fuß: flach auf dem Tisch
    hull() for (dx = [-stand_pad[0] / 2, stand_pad[0] / 2 - stand_pad[1]])
        translate(st_local([stand_foot_x + dx, 0])) rotate(stand_angle)
            square([stand_pad[1], stand_pad[1]]);
}

// Druckteil: flach liegend
module stand_print() {
    linear_extrude(stand_w) stand_2d();
}

// in Einbaulage für die Baugruppenansicht
module stand_assembled() {
    translate([-stand_w / 2 + socket_c[0], outer_r[2], 0])
        multmatrix([[0, 0, 1, 0], [0, 1, 0, 0], [1, 0, 0, 0], [0, 0, 0, 1]])
            linear_extrude(stand_w) stand_2d();
}

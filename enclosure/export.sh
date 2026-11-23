#!/usr/bin/env bash
# SPDX-License-Identifier: CC-BY-SA-4.0
#
# Exportiert alle Druckteile (STL + 3MF) nach exports/ und rendert die
# Vorschaubilder nach ../docs/previews/.
#
#   ./export.sh                 # 7,5" (Standard)
#   ./export.sh 4.2             # 4,2"-Variante
#   NAME="Jürgen" ./export.sh   # anderer Namenszug
#   PREVIEWS=0 ./export.sh      # ohne Vorschaubilder
#
# Bricht ab, wenn OpenSCAD eine Warnung/einen Fehler meldet oder ein Teil
# nicht als einfaches (mannigfaltiges) Volumen gerendert wird.

set -euo pipefail
cd "$(dirname "$0")"

DISPLAY_SIZE="${1:-7.5}"
NAME="${NAME:-since}"
PREVIEWS="${PREVIEWS:-1}"
OPENSCAD="${OPENSCAD:-openscad}"
OUT=exports
PREV=../docs/previews
mkdir -p "$OUT" build "$PREV"

# Headless-Rendering für PNGs braucht unter Linux ohne Bildschirm xvfb-run
RUN_GL=()
if [ -z "${DISPLAY:-}" ] && command -v xvfb-run >/dev/null 2>&1 && [ "$(uname)" = "Linux" ]; then
    RUN_GL=(xvfb-run -a)
fi

defs=(-D "display=\"$DISPLAY_SIZE\"" -D "name_text=\"$NAME\"")

check_log() {
    local log="$1"
    if grep -qiE "^(WARNING|ERROR)|CGAL error|assert" "$log"; then
        echo "FEHLER in $log:" >&2
        grep -iE "WARNING|ERROR|assert" "$log" >&2
        exit 1
    fi
}

for part in front back stand text bay_test; do
    base="$OUT/since-$DISPLAY_SIZE-$part"
    for ext in stl 3mf; do
        log="build/since-$DISPLAY_SIZE-$part.$ext.log"
        "$OPENSCAD" "${defs[@]}" -D "part=\"$part\"" -o "$base.$ext" since.scad >"$log" 2>&1
        check_log "$log"
    done
    # Mannigfaltigkeit: CGAL meldet "Simple: yes" für saubere Nef-Polyeder.
    # (Teile ohne Boolesche Operationen werden ohne CGAL exportiert.)
    log="build/since-$DISPLAY_SIZE-$part.stl.log"
    if grep -q "Simple:" "$log"; then
        grep -q "Simple:        yes" "$log" || { echo "$part ist nicht mannigfaltig" >&2; exit 1; }
    fi
    echo "ok  $base.{stl,3mf}  $(grep -m1 -E 'Volumes|Facets' "$log" | tr -s ' ')"
done

[ "$PREVIEWS" = "1" ] || exit 0

png() {  # png <datei> <part> <kamera> [weitere -D]
    local file="$1" part="$2" cam="$3"; shift 3
    "${RUN_GL[@]}" "$OPENSCAD" "${defs[@]}" -D "part=\"$part\"" "$@" \
        --imgsize=1600,1200 --colorscheme=Tomorrow --camera="$cam" \
        -o "$PREV/$file" since.scad >"build/$file.log" 2>&1
    check_log "build/$file.log"
    echo "ok  $PREV/$file"
}

sfx=""
[ "$DISPLAY_SIZE" = "7.5" ] || sfx="-$DISPLAY_SIZE"
png "enclosure-front$sfx.png"    front    0,0,0,0,180,0,420         --render
png "enclosure-back-inside$sfx.png" back  0,0,0,35,0,0,400          --render
png "enclosure-back-outside$sfx.png" back 0,0,0,145,0,0,400         --render
png "enclosure-stand$sfx.png"    stand    0,0,0,30,0,20,330         --render --viewall --autocenter
png "enclosure-exploded$sfx.png" all      0,0,20,55,0,150,480       -D explode=40 --viewall --autocenter
png "enclosure-standing$sfx.png" standing 40,-600,60,40,0,60        --projection=o
png "enclosure-standing-3d$sfx.png" standing -330,-260,190,20,0,55

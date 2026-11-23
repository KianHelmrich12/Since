# SPDX-License-Identifier: GPL-3.0-or-later
# Geräte-Emulator: spielt die Firmware-Weckzyklen im Zeitraffer durch (gleiche Logik wie
# src/main.cpp über since::runCycle) und rendert jede Neuzeichnung als Bild.
#
#   pio test -e native                      # lädt ArduinoJson (einmalig)
#   python3 tools/sim.py --config ../shared/fixtures/config/valid/full.json \
#       --start 2026-03-10T07:00 --days 7 --out sim-out
#   python3 tools/sim.py --config … --until-empty      # Akkulaufzeit bis „Bitte laden“
#
# Weitere Optionen werden an das Emulator-Programm durchgereicht, z. B.
#   --press 2026-03-11T10:00=key2   (Taste 3: Layout wechseln; key1 = neu zeichnen)
#   --sleep-ua 35 --draw-ms 4500 --draw-ma 35 --capacity-mah 2000 --frames 20
# Die Energie-Parameter sind Annahmen, bis docs/power.md echte Messwerte hat.
import argparse
import glob
import os
import shutil
import subprocess
import sys
import tempfile

HERE = os.path.dirname(os.path.abspath(__file__))
ROOT = os.path.dirname(HERE)
sys.path.insert(0, HERE)
import preview  # noqa: E402  (Renderer und Compiler-Suche teilen)


def build():
    cxx = shutil.which("c++") or shutil.which("g++") or shutil.which("clang++")
    if not cxx:
        sys.exit("Kein C++-Compiler gefunden")
    aj = os.path.join(ROOT, ".pio", "libdeps", "native", "ArduinoJson", "src")
    if not os.path.isdir(aj):
        sys.exit("ArduinoJson fehlt – vorher `pio test -e native` ausführen")
    core = os.path.join(ROOT, "lib", "since_core", "src")
    exe = os.path.join(tempfile.mkdtemp(), "since_sim")
    srcs = [os.path.join(HERE, "sim", "sim.cpp")] + glob.glob(os.path.join(core, "*.cpp"))
    subprocess.run([cxx, "-std=c++17", "-O2", "-I", core, "-I", aj, "-o", exe] + srcs, check=True)
    return exe


LEVEL = {"0": "ok", "1": "niedrig", "2": "kritisch"}


def main():
    ap = argparse.ArgumentParser(add_help=True)
    ap.add_argument("--out", help="Ordner für Bilder (frame-*.png, frames.gif) und timeline.tsv")
    ap.add_argument("--quiet", action="store_true", help="keine Zeitleiste ausgeben")
    args, rest = ap.parse_known_args()
    exe = build()
    out = subprocess.run([exe] + rest, capture_output=True, text=True)
    if out.returncode != 0:
        sys.exit(out.stderr.strip())

    frames, summary, timeline = {}, {}, []
    for line in out.stdout.splitlines():
        parts = line.split("\t")
        if parts[0] == "frame":
            frames.setdefault(int(parts[1]), {})[parts[2]] = parts[3] if len(parts) > 3 else ""
        elif parts[0] == "summary":
            summary[parts[1]] = parts[2]
        elif parts[0] == "wake":
            timeline.append(parts[1:])

    if not args.quiet:
        print("Zeitpunkt (lokal)    Akku     Stufe     Neu gezeichnet  Grund")
        shown = timeline if len(timeline) <= 60 else timeline[:40] + [None] + timeline[-10:]
        for t in shown:
            if t is None:
                print("  … (%d Zyklen ausgelassen) …" % (len(timeline) - 50))
                continue
            _, local, mv, lvl, draw, reason = t
            print("%-20s %4d mV  %-9s %-15s %s" % (local, int(mv), LEVEL[lvl], "ja" if draw == "1" else "–", reason))
        print()

    labels = [
        ("days", "Simulierte Tage"), ("wakes", "Weckzyklen"), ("draws", "Neuzeichnungen"),
        ("wakes_per_day", "Weckzyklen pro Tag"), ("mah_per_day", "Verbrauch mAh/Tag"),
        ("sleep_mah_per_day", "  davon Schlaf"), ("active_mah_per_day", "  davon aktiv"),
        ("selfdischarge_mah_per_day", "  davon Selbstentladung"),
        ("battery_percent_end", "Akku am Ende %"), ("empty_after_days", "„Bitte laden“ nach Tagen"),
        ("estimated_runtime_days", "Hochgerechnete Laufzeit (Tage)"),
    ]
    for k, label in labels:
        print("%-32s %s" % (label, summary.get(k, "?")))

    if args.out:
        os.makedirs(args.out, exist_ok=True)
        images = []
        for n in sorted(frames):
            img = preview.render(frames[n])
            path = os.path.join(args.out, "frame-%03d.png" % n)
            img.save(path)
            images.append(img.convert("L"))
        if images:
            images[0].save(os.path.join(args.out, "frames.gif"), save_all=True,
                           append_images=images[1:], duration=1200, loop=0)
        with open(os.path.join(args.out, "timeline.tsv"), "w", encoding="utf-8") as f:
            f.write("utc\tlocal\tbattery_mv\tlevel\tdraw\treason\n")
            for t in timeline:
                f.write("\t".join(t) + "\n")
        print("\n%d Bilder und timeline.tsv in %s" % (len(images), args.out))


if __name__ == "__main__":
    main()

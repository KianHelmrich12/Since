# SPDX-License-Identifier: GPL-3.0-or-later
# Host-Renderer: erzeugt Vorschaubilder aller Screens nach docs/previews/display-*.png.
#   pio test -e native        (lädt ArduinoJson nach .pio/libdeps/native)
#   python3 tools/preview.py
# Die Texte stammen aus since_core (echtes Bildschirm-Modell); Fonts und Positionen sind eine
# Annäherung an src/display.cpp (dort U8g2-Fonts). Maßgeblich bleibt das Gerät.
# Benötigt: C++17-Compiler (c++/g++/clang++), Pillow, eine TrueType-Sans-Schrift.
import glob
import os
import shutil
import subprocess
import sys
import tempfile

from PIL import Image, ImageDraw, ImageFont

ROOT = os.path.dirname(os.path.dirname(os.path.abspath(__file__)))
OUT = os.path.join(os.path.dirname(ROOT), "docs", "previews")
W, H, M = 800, 480, 24
KINDS = ["standard", "minimal", "milestone", "battery", "setup", "settime", "welcome"]


def find_font(bold):
    names = ["LiberationSans-Bold.ttf", "DejaVuSans-Bold.ttf", "Arial Bold.ttf", "arialbd.ttf"] if bold \
        else ["LiberationSans-Regular.ttf", "DejaVuSans.ttf", "Arial.ttf", "arial.ttf"]
    dirs = ["/usr/share/fonts", "/usr/local/share/fonts", "/Library/Fonts", "/System/Library/Fonts",
            "C:/Windows/Fonts", os.path.expanduser("~/.fonts")]
    for d in dirs:
        for n in names:
            hits = glob.glob(os.path.join(d, "**", n), recursive=True)
            if hits:
                return hits[0]
    sys.exit("Keine TrueType-Schrift gefunden (Liberation Sans oder DejaVu Sans installieren)")


BOLD, REG = find_font(True), find_font(False)


def font(px, bold=False):
    return ImageFont.truetype(BOLD if bold else REG, px)


def build_dump():
    cxx = shutil.which("c++") or shutil.which("g++") or shutil.which("clang++")
    if not cxx:
        sys.exit("Kein C++-Compiler gefunden")
    aj = os.path.join(ROOT, ".pio", "libdeps", "native", "ArduinoJson", "src")
    if not os.path.isdir(aj):
        sys.exit("ArduinoJson fehlt – vorher `pio test -e native` ausführen")
    core = os.path.join(ROOT, "lib", "since_core", "src")
    exe = os.path.join(tempfile.mkdtemp(), "preview_dump")
    srcs = [os.path.join(ROOT, "tools", "preview_dump.cpp")] + glob.glob(os.path.join(core, "*.cpp"))
    subprocess.run([cxx, "-std=c++17", "-O1", "-I", core, "-I", aj, "-o", exe] + srcs, check=True)
    out = subprocess.run([exe], check=True, capture_output=True, text=True).stdout
    screens = {}
    for line in out.splitlines():
        sid, key, val = (line.split("\t") + ["", ""])[:3]
        screens.setdefault(sid, {})[key] = val
    return screens


def fit(draw, text, sizes, bold, max_w):
    for s in sizes:
        f = font(s, bold)
        if draw.textlength(text, font=f) <= max_w:
            return f
    return font(sizes[-1], bold)


def centered(draw, text, y, f, fill=0):
    if text:
        draw.text(((W - draw.textlength(text, font=f)) / 2, y), text, font=f, fill=fill, anchor="ls")


def battery(draw, pct, warn):
    if pct < 0:
        return
    x, y, w, h = W - M - 44, 14, 40, 18
    draw.rectangle([x, y, x + w - 1, y + h - 1], outline=0)
    draw.rectangle([x + w, y + 5, x + w + 3, y + h - 6], fill=0)
    fw = (w - 4) * pct // 100
    if fw > 0:
        draw.rectangle([x + 2, y + 2, x + 2 + fw - 1, y + h - 3], fill=0)
    if warn:
        draw.text((x - 14, y + h - 2), "!", font=font(20, True), fill=0, anchor="ls")


def banner(draw, text):
    if not text:
        return
    y = H - 84
    draw.rectangle([0, y, W, H], fill=0)
    centered(draw, text, y + 54, fit(draw, text, [34, 26], True, W - 2 * M), fill=255)


def render(s):
    img = Image.new("L", (W, H), 255)
    d = ImageDraw.Draw(img)
    kind = KINDS[int(s["kind"])]
    mw = W - 2 * M
    if kind in ("standard", "milestone"):
        centered(d, s["subtitle"], 64, fit(d, s["subtitle"], [34, 26, 20], False, mw))
        centered(d, s["title"], 160, fit(d, s["title"], [84, 66, 54, 42], True, mw))
        centered(d, s["headline"], 250, fit(d, s["headline"], [48, 40, 34, 26], False, mw))
        centered(d, s["line1"], 310, font(26))
        centered(d, s["line2"], 354, font(20))
        banner(d, s["banner"])
    elif kind == "minimal":
        centered(d, s["title"], 110, fit(d, s["title"], [48, 40, 34], True, mw))
        centered(d, s["headline"], 270, fit(d, s["headline"], [72, 60, 48, 40], False, mw))
        centered(d, s["line1"], H - 40, font(20))
        banner(d, s["banner"])
    elif kind == "setup":
        q = 360
        qx, qy = W - M - q, (H - q) // 2
        d.rectangle([qx + 20, qy + 20, qx + q - 20, qy + q - 20], outline=0, width=3)
        d.text((qx + q / 2, qy + q / 2), "QR-Code", font=font(26), fill=0, anchor="mm")
        x = M + 8
        d.text((x, 90), s["title"], font=font(48, True), fill=0, anchor="ls")
        d.text((x, 160), "1. QR-Code scannen oder", font=font(26, True), fill=0, anchor="ls")
        d.text((x, 196), s["subtitle"], font=font(26, True), fill=0, anchor="ls")
        d.text((x, 232), s["headline"], font=font(26), fill=0, anchor="ls")
        d.text((x, 290), "2. " + s["line1"], font=font(20), fill=0, anchor="ls")
        d.text((x, 350), s["line2"], font=font(20), fill=0, anchor="ls")
        d.text((x, H - 40), "Ende nach 10 min ohne Aktivität.", font=font(20), fill=0, anchor="ls")
    else:
        centered(d, s["title"], 190, fit(d, s["title"], [84, 66, 54, 42], True, mw))
        centered(d, s["subtitle"], 260, fit(d, s["subtitle"], [34, 26, 20], False, mw))
        centered(d, s["line1"], 330, font(26))
        centered(d, s["line2"], 370, font(26))
    if kind != "setup":
        battery(d, int(s["battery"]), s["warn"] == "1")
    return img.convert("1")


def main():
    os.makedirs(OUT, exist_ok=True)
    for sid, s in build_dump().items():
        path = os.path.join(OUT, "display-%s.png" % sid)
        render(s).save(path)
        print(path)


if __name__ == "__main__":
    main()

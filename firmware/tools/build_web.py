# SPDX-License-Identifier: GPL-3.0-or-later
# PlatformIO pre-Skript: komprimiert firmware/web/* nach firmware/data/*.gz (für LittleFS).
# Läuft auch eigenständig: python3 tools/build_web.py
import gzip
import os
import sys

LIMIT = 30 * 1024  # Ziel aus CLAUDE.md: Portal-Assets < 30 KB (komprimiert)


def build(project_dir):
    src = os.path.join(project_dir, "web")
    dst = os.path.join(project_dir, "data")
    os.makedirs(dst, exist_ok=True)
    total = 0
    for name in sorted(os.listdir(src)):
        path = os.path.join(src, name)
        if not os.path.isfile(path):
            continue
        with open(path, "rb") as f:
            raw = f.read()
        out = os.path.join(dst, name + ".gz")
        # mtime=0 → reproduzierbare Ausgabe
        with open(out, "wb") as f:
            with gzip.GzipFile(filename="", mode="wb", fileobj=f, compresslevel=9, mtime=0) as gz:
                gz.write(raw)
        total += os.path.getsize(out)
    print("web: %d Bytes gzip in data/ (Limit %d)" % (total, LIMIT))
    if total > LIMIT:
        sys.exit("web: Portal-Assets überschreiten %d Bytes" % LIMIT)
    return total


try:
    Import("env")  # noqa: F821 – von PlatformIO bereitgestellt
    build(env["PROJECT_DIR"])  # noqa: F821
except NameError:
    build(os.path.dirname(os.path.dirname(os.path.abspath(__file__))))

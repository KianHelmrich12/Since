# SPDX-License-Identifier: GPL-3.0-or-later
# Virtuelles since-Gerät im Browser – ohne Hardware.
#
#   cd firmware
#   pio run -e ee04          # einmalig: lädt die Bibliotheken (GFX, U8g2, QRCode, ArduinoJson)
#   python3 tools/emulator.py            → http://localhost:8080/
#
# Der Emulator nutzt dieselbe Logik und dasselbe Rendering wie die Firmware:
#   since_core (runCycle, callApi, handleRpc) + src/render.cpp mit den echten U8g2-Fonts.
# Simuliert werden: Uhr (Zeitraffer), DS3231 inkl. Stromverlust, Akku (Entladung/Laden),
# NVS (Datei), Tasten, Setup-Modus mit Portal (/portal/) und BLE-RPC für die App (/api/rpc).
# Nicht emuliert: Funk, E-Paper-Wellenformen, Stromverbrauch einzelner Bauteile, OTA.
import argparse
import ctypes
import glob
import hashlib
import io
import json
import os
import shutil
import subprocess
import sys
import threading
import time
from http.server import BaseHTTPRequestHandler, ThreadingHTTPServer
from urllib.parse import parse_qs, urlparse

from PIL import Image

HERE = os.path.dirname(os.path.abspath(__file__))
ROOT = os.path.dirname(HERE)
LIBDEPS = os.path.join(ROOT, ".pio", "libdeps", "ee04")
BUILD = os.path.join(ROOT, ".pio", "emulator")

# Energie-Annahmen wie tools/sim (bis docs/power.md echte Messwerte hat)
SLEEP_UA = 20.0
WAKE_MAS = 0.3 * 40
DRAW_MAS = 4.5 * 35
SETUP_MA = 110.0     # WLAN-AP + BLE aktiv
CHARGE_MA = 500.0
CAPACITY_MAS = 2000 * 3600.0


def lib_src(*parts):
    p = os.path.join(LIBDEPS, *parts)
    if not os.path.exists(p):
        sys.exit("Bibliothek fehlt: %s – vorher `pio run -e ee04` ausführen" % p)
    return p


def build():
    cxx = shutil.which("c++") or shutil.which("g++") or shutil.which("clang++")
    cc = shutil.which("cc") or shutil.which("gcc") or shutil.which("clang")
    if not cxx or not cc:
        sys.exit("C/C++-Compiler fehlt")
    core = os.path.join(ROOT, "lib", "since_core", "src")
    gfx = lib_src("Adafruit GFX Library")
    u8 = lib_src("U8g2_for_Adafruit_GFX", "src")
    qr = lib_src("QRCode", "src")
    aj = lib_src("ArduinoJson", "src")
    cpp = (glob.glob(os.path.join(core, "*.cpp")) + [os.path.join(ROOT, "src", "render.cpp"),
           os.path.join(ROOT, "emu", "emu_device.cpp"), os.path.join(gfx, "Adafruit_GFX.cpp"),
           os.path.join(u8, "U8g2_for_Adafruit_GFX.cpp")])
    c = [os.path.join(u8, "u8g2_fonts.c"), os.path.join(qr, "qrcode.c")]
    flags = ["-DARDUINOJSON_ENABLE_ARDUINO_STRING=0", "-DARDUINOJSON_ENABLE_ARDUINO_STREAM=0",
             "-DARDUINOJSON_ENABLE_ARDUINO_PRINT=0", "-DARDUINOJSON_ENABLE_PROGMEM=0"]
    inc = flags + ["-I", os.path.join(ROOT, "emu", "shim"), "-I", core, "-I", os.path.join(ROOT, "src"),
           "-I", gfx, "-I", u8, "-I", qr, "-I", aj]
    h = hashlib.sha1()
    for f in sorted(cpp + c + glob.glob(os.path.join(core, "*.h")) + [os.path.join(ROOT, "src", "render.h")]):
        h.update(open(f, "rb").read())
    os.makedirs(BUILD, exist_ok=True)
    so = os.path.join(BUILD, "since_emu_%s.so" % h.hexdigest()[:12])
    if os.path.exists(so):
        return so
    print("Baue Emulator-Bibliothek …", flush=True)
    objs = []
    for f in c:
        o = os.path.join(BUILD, os.path.basename(f) + ".o")
        subprocess.run([cc, "-std=gnu11", "-O2", "-fPIC", "-w", "-c", f, "-o", o] + inc, check=True)  # C-Standard fest: qrcode.h definiert bool selbst (in C23 ein Schlüsselwort)
        objs.append(o)
    subprocess.run([cxx, "-std=c++17", "-O2", "-fPIC", "-shared", "-w", "-o", so] + inc + cpp + objs, check=True)
    return so


class Device:
    """Hält die simulierte Welt und ruft die Emulator-Bibliothek serialisiert auf."""

    def __init__(self, so, state_path, speed):
        self.lib = ctypes.CDLL(so)
        L = self.lib
        L.emu_init.argtypes = [ctypes.c_char_p]
        L.emu_set_world.argtypes = [ctypes.c_longlong, ctypes.c_int, ctypes.c_int]
        for fn in ("emu_wake", "emu_setup_tick", "emu_setup_info", "emu_rpc"):
            getattr(L, fn).restype = ctypes.c_char_p
        L.emu_wake.argtypes = [ctypes.c_char_p]
        L.emu_rpc.argtypes = [ctypes.c_char_p]
        L.emu_api.argtypes = [ctypes.c_char_p, ctypes.c_char_p, ctypes.c_char_p]
        L.emu_api.restype = ctypes.c_char_p
        L.emu_framebuffer.restype = ctypes.POINTER(ctypes.c_uint8)
        L.emu_framebuffer_size.restype = ctypes.c_int
        self.lock = threading.RLock()
        L.emu_init(state_path.encode())
        self.world = time.time()     # Weltzeit (UTC, Sekunden)
        self.last_real = time.monotonic()
        self.speed = speed
        self.charge = CAPACITY_MAS * 0.8
        self.usb = False
        self.internet = False
        self.awake_state = {}
        self.next_wake = 0
        self.frame = 0
        self.png = b""
        self.log = []
        self.wake("power_on")

    # --- Akku
    def mv(self):
        pct = 100.0 * self.charge / CAPACITY_MAS
        lo, hi = 3000, 4200
        # Umkehrung der Kennlinie aus since_core (grob, monoton)
        curve = [(4200, 100), (4150, 95), (4110, 90), (4080, 85), (4020, 80), (3980, 75), (3950, 70),
                 (3910, 65), (3870, 60), (3850, 55), (3840, 50), (3820, 45), (3800, 40), (3790, 35),
                 (3770, 30), (3750, 25), (3730, 20), (3710, 15), (3690, 10), (3610, 5), (3500, 3), (3300, 0)]
        for (mh, ph), (ml, pl) in zip(curve, curve[1:]):
            if pct >= pl:
                return int(ml + (pct - pl) * (mh - ml) / max(ph - pl, 1))
        return lo

    def sync_world(self):
        self.lib.emu_set_world(int(self.world), self.mv(), 1 if self.internet else 0)

    def add_log(self, text):
        if text:
            for line in text.split("\n"):
                self.log.append([int(self.world), line])  # im Browser lokal formatiert
            self.log = self.log[-200:]

    def apply(self, raw):
        r = json.loads(raw)
        self.add_log(r.get("log", ""))
        if r.get("drawn"):
            self.charge -= DRAW_MAS
            self.frame += 1
            self.render_png()
        self.charge -= WAKE_MAS
        if "nextWakeUtc" in r:
            self.next_wake = r["nextWakeUtc"]
        self.awake_state = r
        return r

    def render_png(self):
        n = self.lib.emu_framebuffer_size()
        buf = ctypes.string_at(self.lib.emu_framebuffer(), n)
        img = Image.frombytes("1", (800, 480), buf)
        out = io.BytesIO()
        img.save(out, "PNG")
        self.png = out.getvalue()

    def wake(self, reason):
        with self.lock:
            self.sync_world()
            return self.apply(self.lib.emu_wake(reason.encode()))

    # --- Zeit
    def advance(self, seconds):
        """Weltzeit vorspulen und dabei alle fälligen Weckzeitpunkte abarbeiten."""
        with self.lock:
            target = self.world + seconds
            while True:
                setup = self.awake_state.get("setup")
                if setup:
                    step = min(target - self.world, 5.0)
                else:
                    step = (self.next_wake - self.world) if self.next_wake else target - self.world
                    step = min(step, target - self.world)
                if step > 0:
                    self.drain(step, setup)
                    self.world += step
                if setup:
                    self.sync_world()
                    r = json.loads(self.lib.emu_setup_tick())
                    if r.get("log") or r.get("drawn"):
                        self.apply(json.dumps(r))
                    if self.world >= target:
                        break
                    continue
                if self.next_wake and self.world >= self.next_wake:
                    self.wake("rtc_alarm")
                    continue
                if self.world >= target:
                    break

    def drain(self, seconds, setup):
        if self.usb:
            self.charge = min(CAPACITY_MAS, self.charge + CHARGE_MA * seconds)
        else:
            ma = SETUP_MA if setup else SLEEP_UA / 1000.0
            self.charge = max(0.0, self.charge - ma * seconds)

    def tick(self):
        now = time.monotonic()
        dt = (now - self.last_real) * self.speed
        self.last_real = now
        self.advance(dt)

    # --- APIs
    def api(self, op, params="{}", code=""):
        with self.lock:
            self.sync_world()
            r = json.loads(self.lib.emu_api(op.encode(), params.encode(), code.encode()))
            if op == "factoryReset" and r["http"] == 200:
                self.frame += 1
                self.render_png()
            return r

    def rpc(self, body):
        with self.lock:
            self.sync_world()
            return self.lib.emu_rpc(body.encode()).decode()

    def state(self):
        with self.lock:
            info = json.loads(self.lib.emu_setup_info())
            s = self.awake_state
            return {
                "worldUtc": int(self.world), "speed": self.speed, "frame": self.frame,
                "batteryMv": self.mv(), "batteryPercent": round(100 * self.charge / CAPACITY_MAS, 1),
                "usb": self.usb, "internet": self.internet,
                "nextWakeUtc": self.next_wake, "rtcUtc": s.get("rtcUtc", 0),
                "rtcLostPower": s.get("rtcLostPower", False), "setup": info, "log": self.log[-60:],
            }


EMU_HTML = os.path.join(HERE, "emulator", "index.html")
WEB = os.path.join(ROOT, "web")
TYPES = {".html": "text/html; charset=utf-8", ".css": "text/css", ".js": "application/javascript"}
PORTAL_API = {("GET", "/api/config"): "getConfig", ("POST", "/api/config"): "setConfig",
              ("POST", "/api/time"): "setTime", ("GET", "/api/status"): "getStatus",
              ("POST", "/api/redraw"): "redraw", ("POST", "/api/exit"): "exitSetup",
              ("POST", "/api/factory-reset"): "factoryReset", ("POST", "/api/ota"): "ota"}


def make_handler(dev):
    class H(BaseHTTPRequestHandler):
        def cors(self):
            self.send_header("Access-Control-Allow-Origin", "*")
            self.send_header("Access-Control-Allow-Headers", "Content-Type, X-Setup-Code")

        def send(self, code, body, ctype="application/json"):
            data = body if isinstance(body, bytes) else body.encode()
            self.send_response(code)
            self.send_header("Content-Type", ctype)
            self.send_header("Content-Length", str(len(data)))
            self.send_header("Cache-Control", "no-store")
            self.cors()
            self.end_headers()
            self.wfile.write(data)

        def do_OPTIONS(self):
            self.send_response(204)
            self.cors()
            self.end_headers()

        def body(self):
            n = int(self.headers.get("Content-Length", 0))
            return self.rfile.read(n).decode("utf-8", "replace") if n else ""

        def static(self, path, base):
            f = os.path.join(base, os.path.basename(path) or "index.html")
            if not os.path.isfile(f):
                return self.send(404, "{}")
            self.send(200, open(f, "rb").read(), TYPES.get(os.path.splitext(f)[1], "text/plain"))

        def portal_api(self, method, path):
            op = PORTAL_API.get((method, path))
            if not op:
                return self.send(404, '{"error":"unknown_op"}')
            params = self.body() if method == "POST" else "{}"
            if op == "factoryReset" and '"confirm":"RESET"' not in params.replace(" ", ""):
                return self.send(400, '{"error":"bad_request"}')
            r = dev.api(op, params or "{}", self.headers.get("X-Setup-Code", ""))
            self.send(r["http"], r["body"])

        def do_GET(self):
            u = urlparse(self.path)
            if u.path in ("/", "/index.html"):
                return self.static("index.html", os.path.dirname(EMU_HTML))
            if u.path == "/emu/state":
                return self.send(200, json.dumps(dev.state()))
            if u.path == "/emu/screen.png":
                return self.send(200, dev.png, "image/png")
            if u.path.startswith("/portal"):
                return self.static(u.path[len("/portal"):].lstrip("/"), WEB)
            if u.path in ("/app.js", "/style.css"):  # vom Portal relativ geladen
                return self.static(u.path, WEB)
            if u.path.startswith("/api/"):
                return self.portal_api("GET", u.path)
            self.send(404, "{}")

        def do_POST(self):
            u = urlparse(self.path)
            q = parse_qs(u.query)
            if u.path == "/emu/press":
                key = q.get("key", [""])[0]
                if key not in ("key0", "key0_hold", "key1", "key2", "power_on"):
                    return self.send(400, "{}")
                dev.wake(key)
                return self.send(200, "{}")
            if u.path == "/emu/control":
                with dev.lock:
                    if "speed" in q:
                        dev.speed = max(0.0, float(q["speed"][0]))
                    if "advance" in q:
                        dev.advance(float(q["advance"][0]))
                    if "battery" in q:
                        dev.charge = CAPACITY_MAS * max(0.0, min(100.0, float(q["battery"][0]))) / 100
                    if "usb" in q:
                        dev.usb = q["usb"][0] == "1"
                    if "internet" in q:
                        dev.internet = q["internet"][0] == "1"
                    if q.get("coincell") == ["remove"]:
                        dev.lib.emu_rtc_lose_power()
                        dev.add_log("Knopfzelle entfernt: RTC hat die Zeit verloren")
                        dev.wake("power_on")
                return self.send(200, "{}")
            if u.path == "/api/rpc":
                return self.send(200, dev.rpc(self.body()))
            if u.path.startswith("/api/"):
                return self.portal_api("POST", u.path)
            self.send(404, "{}")

        def log_message(self, *a):
            pass

    return H


def main():
    ap = argparse.ArgumentParser(description="Virtuelles since-Gerät")
    ap.add_argument("--port", type=int, default=8080)
    ap.add_argument("--speed", type=float, default=1.0, help="Zeitraffer-Faktor (1 = Echtzeit)")
    ap.add_argument("--state", default=os.path.join(BUILD, "nvs.json"),
                    help="Datei für den emulierten NVS (Einstellungen bleiben erhalten)")
    args = ap.parse_args()
    so = build()
    os.makedirs(os.path.dirname(args.state), exist_ok=True)
    dev = Device(so, args.state, args.speed)

    def loop():
        while True:
            time.sleep(0.25)
            try:
                dev.tick()
            except Exception as e:  # Emulator soll weiterlaufen
                print("tick:", e, file=sys.stderr)

    threading.Thread(target=loop, daemon=True).start()
    print("since-Emulator: http://localhost:%d/" % args.port, flush=True)
    ThreadingHTTPServer(("127.0.0.1", args.port), make_handler(dev)).serve_forever()


if __name__ == "__main__":
    main()

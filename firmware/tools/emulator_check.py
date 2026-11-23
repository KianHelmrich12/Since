# SPDX-License-Identifier: GPL-3.0-or-later
# Automatischer Test des Geräte-Emulators (läuft in der CI nach `pio run -e ee04`):
#  1. Alle Gerätefonts enthalten die nötigen Zeichen (Umlaute, ß, ·, », «, ASCII).
#  2. Durchlauf: Einschalten → Taste 1 → Config + Uhrzeit per API → Fertig → Display zeigt
#     den Namen; Zeitraffer bis zum Geburtstag; Taste 3; Knopfzelle raus.
import json
import os
import sys
import tempfile

sys.path.insert(0, os.path.dirname(os.path.abspath(__file__)))
import emulator  # noqa: E402

REQUIRED = ("ABCDEFGHIJKLMNOPQRSTUVWXYZabcdefghijklmnopqrstuvwxyz0123456789"
            ".,:;!?()-/+&'\"%»«·äöüÄÖÜß")

failures = []


def check(cond, msg):
    print(("ok    " if cond else "FAIL  ") + msg)
    if not cond:
        failures.append(msg)


def white_ratio(dev):
    buf = emulator.ctypes.string_at(dev.lib.emu_framebuffer(), dev.lib.emu_framebuffer_size())
    ones = sum(bin(b).count("1") for b in buf)
    return ones / (len(buf) * 8)


def main():
    so = emulator.build()
    state = os.path.join(tempfile.mkdtemp(), "nvs.json")
    dev = emulator.Device(so, state, speed=0)
    dev.lib.emu_missing_glyphs.argtypes = [emulator.ctypes.c_char_p]
    dev.lib.emu_missing_glyphs.restype = emulator.ctypes.c_char_p

    missing = dev.lib.emu_missing_glyphs(REQUIRED.encode()).decode()
    check(missing == "", "Fonts enthalten alle Pflichtzeichen" + ("" if not missing else ":\n" + missing))

    check(dev.awake_state["screen"]["kind"] == "welcome", "Einschalten zeigt den Willkommens-Screen")
    r = dev.wake("key0")
    check(r["setup"] and r["screen"]["kind"] == "setup", "Taste 1 startet den Setup-Modus")
    info = json.loads(dev.lib.emu_setup_info())

    resp = dev.api("setConfig", json.dumps({"name": "Jörg", "occasion": "seit deiner Geburt",
                                             "event": "2023-03-12T08:15"}))
    check(resp["http"] == 200, "Config per Portal-API gespeichert")
    resp = dev.api("setConfig", json.dumps({"wifi": {"ssid": "x", "password": "kurz"}}))
    check(resp["http"] == 400 and "wifi.password" in resp["body"], "ungültige Config wird abgelehnt")
    resp = dev.api("setTime", json.dumps({"epoch": int(dev.world)}))
    check(resp["http"] == 200, "Uhrzeit gesetzt")
    rpc = json.loads(dev.rpc(json.dumps({"id": 1, "op": "getInfo"})))
    check(rpc["ok"] and rpc["data"]["proto"] == 1, "BLE-RPC antwortet")
    check(dev.api("factoryReset", "{}", "000000")["http"] == 403, "Werksreset ohne Code verweigert")
    dev.api("exitSetup")
    dev.advance(5)
    s = dev.awake_state
    check(not s["setup"] and s["screen"]["title"] == "Jörg", "Nach „Fertig“ zeigt das Display den Namen")
    check("·" in s["screen"]["headline"], "Kopfzeile enthält Trennpunkte: " + s["screen"]["headline"])
    ratio = white_ratio(dev)
    check(0.80 < ratio < 0.995, "Bild enthält Text (Weißanteil %.3f)" % ratio)

    # Zeitraffer bis zum nächsten 12.03. 10:00 MEZ → Geburtstags-Banner
    import datetime as dt
    now = dt.datetime.fromtimestamp(dev.world, dt.timezone.utc)
    year = now.year if (now.month, now.day) < (3, 12) else now.year + 1
    target = dt.datetime(year, 3, 12, 9, 0, tzinfo=dt.timezone.utc).timestamp()
    frames = dev.frame
    dev.advance(target - dev.world)
    s = dev.awake_state
    check("Geburtstag" in s["screen"]["banner"], "Geburtstags-Banner am 12.03.: " + s["screen"]["banner"])
    days = (target - now.timestamp()) / 86400
    check(dev.frame - frames <= days * 2 + 3, "höchstens ~1–2 Neuzeichnungen pro Tag (%d in %.0f Tagen)"
          % (dev.frame - frames, days))

    r = dev.wake("key2")
    check(r["screen"]["kind"] == "minimal", "Taste 3 wechselt auf Minimal")
    dev.lib.emu_rtc_lose_power()
    r = dev.wake("power_on")
    check(r["screen"]["kind"] == "settime", "Knopfzelle raus → „Uhrzeit einstellen“")

    print("\n%d Fehler" % len(failures))
    sys.exit(1 if failures else 0)


if __name__ == "__main__":
    main()

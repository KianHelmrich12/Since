# SPDX-License-Identifier: GPL-3.0-or-later
# Mini-Mock der Geräte-API für die Entwicklung der Portal-UI ohne Hardware.
#   python3 tools/portal_mock.py [port]   →  http://localhost:8080/
# Validiert nur grob; maßgeblich ist since_core (pio test -e native).
import json
import os
import sys
import time
from http.server import BaseHTTPRequestHandler, ThreadingHTTPServer

WEB = os.path.join(os.path.dirname(os.path.dirname(os.path.abspath(__file__))), "web")
STATE = {
    "config": {
        "schemaVersion": 1, "name": "Mila", "occasion": "seit deiner Geburt",
        "event": "2023-03-12T08:15", "tz": "CET-1CEST,M3.5.0,M10.5.0/3", "layout": "standard",
        "showTime": False, "update": {"mode": "auto", "intervalMinutes": 60},
        "wifi": {"ssid": "", "passwordSet": False}, "ntp": False, "portalSecure": True,
    },
    "offset": 0,
}
TYPES = {".html": "text/html; charset=utf-8", ".css": "text/css", ".js": "application/javascript"}


def validate(c):
    issues = []
    if len(c.get("name", "")) > 32:
        issues.append({"field": "name", "code": "too_long"})
    pw = c.get("wifi", {}).get("password")
    if pw is not None and pw != "" and not 8 <= len(pw) <= 63:
        issues.append({"field": "wifi.password", "code": "length"})
    if c.get("ntp") and not c.get("wifi", {}).get("ssid"):
        issues.append({"field": "ntp", "code": "requires_wifi"})
    u = c.get("update", {})
    if u.get("mode") == "interval" and not 10 <= int(u.get("intervalMinutes", 0)) <= 1440:
        issues.append({"field": "update.intervalMinutes", "code": "out_of_range"})
    return issues


class Handler(BaseHTTPRequestHandler):
    def _json(self, code, obj):
        body = json.dumps(obj).encode()
        self.send_response(code)
        self.send_header("Content-Type", "application/json")
        self.send_header("Content-Length", str(len(body)))
        self.end_headers()
        self.wfile.write(body)

    def do_GET(self):
        if self.path == "/api/config":
            return self._json(200, STATE["config"])
        if self.path == "/api/status":
            return self._json(200, {
                "fw": "dev", "name": "since-AB12", "batteryMv": 3912, "batteryPercent": 66,
                "charging": None, "rtcOk": True, "rtcLostPower": False,
                "nowUtc": int(time.time()) + STATE["offset"], "lastSync": 0, "setupRemainingS": 540,
            })
        path = "/index.html" if self.path in ("/", "") else self.path.split("?")[0]
        f = os.path.join(WEB, os.path.basename(path))
        if not os.path.isfile(f):
            return self._json(404, {"error": "unknown_op"})
        data = open(f, "rb").read()
        self.send_response(200)
        self.send_header("Content-Type", TYPES.get(os.path.splitext(f)[1], "text/plain"))
        self.end_headers()
        self.wfile.write(data)

    def do_POST(self):
        n = int(self.headers.get("Content-Length", 0))
        raw = self.rfile.read(n)
        try:
            body = json.loads(raw or b"{}")
        except ValueError:
            return self._json(400, {"error": "invalid_json"})
        if self.path == "/api/config":
            merged = json.loads(json.dumps(STATE["config"]))
            for k, v in body.items():
                if isinstance(v, dict):
                    merged.setdefault(k, {}).update(v)
                else:
                    merged[k] = v
            issues = validate(merged)
            if issues:
                return self._json(400, {"error": "invalid", "issues": issues})
            pw = merged["wifi"].pop("password", None)
            if pw is not None:
                merged["wifi"]["passwordSet"] = pw != ""
            STATE["config"] = merged
            return self._json(200, merged)
        if self.path == "/api/time":
            STATE["offset"] = int(body.get("epoch", time.time())) - int(time.time())
            return self._json(200, {"epoch": body.get("epoch")})
        if self.path.startswith(("/api/factory-reset", "/api/ota")):
            # Wie das Gerät: Code vom Display nötig (im Mock: 123456)
            if self.headers.get("X-Setup-Code") != "123456":
                return self._json(403, {"error": "unauthorized"})
            return self._json(200, {})
        if self.path in ("/api/exit", "/api/redraw"):
            return self._json(200, {})
        return self._json(404, {"error": "unknown_op"})

    def log_message(self, *args):
        pass


if __name__ == "__main__":
    port = int(sys.argv[1]) if len(sys.argv) > 1 else 8080
    print("Portal-Mock: http://localhost:%d/" % port)
    ThreadingHTTPServer(("127.0.0.1", port), Handler).serve_forever()

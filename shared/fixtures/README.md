# Fixtures

Gemeinsame Testdaten für die Firmware (`pio test -e native`) und die App (Kontrakttests).

- `config/valid/*.json` werden auf die Default-Config angewendet und sind gültig.
- `config/invalid/*.json` werden abgelehnt (ungültiges JSON, falsche Typen, Validierungsfehler,
  nicht unterstützte Schema-Version).

Die Fixtures enthalten Beispieldaten, keine echten WLAN-Zugangsdaten.

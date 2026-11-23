'use strict';
const $ = (s) => document.querySelector(s);
const f = $('#f');
const MSG = {
  too_long: 'zu lang', invalid: 'ungültig', invalid_utf8: 'ungültige Zeichen',
  out_of_range: '10–1440 Minuten', length: '8–63 Zeichen oder leer',
  requires_wifi: 'braucht eine SSID', wrong_type: 'falscher Typ'
};
const FIELD = {
  name: 'name', occasion: 'occasion', event: 'date', tz: 'tz',
  'update.intervalMinutes': 'intervalMinutes', 'wifi.ssid': 'ssid',
  'wifi.password': 'password', ntp: 'ntp', layout: 'layout'
};

function toast(t, err) {
  const m = $('#msg');
  m.textContent = t; m.className = err ? 'err' : ''; m.style.display = 'block';
  clearTimeout(toast.t); toast.t = setTimeout(() => (m.style.display = 'none'), 4000);
}

async function api(path, body, code) {
  const headers = { 'Content-Type': 'application/json' };
  if (code) headers['X-Setup-Code'] = code;
  const opt = body === undefined ? {} : { method: 'POST', headers, body: JSON.stringify(body) };
  const r = await fetch('/api/' + path, opt);
  const j = await r.json().catch(() => ({}));
  if (!r.ok) throw Object.assign(new Error(j.error || r.status), { data: j });
  return j;
}

function tabs() {
  document.querySelectorAll('nav button').forEach((b) => (b.onclick = () => {
    document.querySelectorAll('nav button').forEach((x) => x.classList.toggle('on', x === b));
    document.querySelectorAll('main section').forEach((s) => (s.hidden = s.id !== b.dataset.tab));
    if (b.dataset.tab === 'status') status();
  }));
}

function syncVisibility() {
  $('#tzc').hidden = f.tzp.value !== '';
  $('#ivl').hidden = f.mode.value !== 'interval';
}

function fill(c) {
  f.name.value = c.name; f.occasion.value = c.occasion;
  const [d, t] = (c.event || '').split('T');
  f.date.value = d || ''; f.time.value = (t || '').slice(0, 5);
  const preset = [...f.tzp.options].some((o) => o.value === c.tz);
  f.tzp.value = preset ? c.tz : ''; f.tz.value = c.tz;
  f.layout.value = c.layout; f.showTime.checked = c.showTime;
  f.mode.value = c.update.mode; f.intervalMinutes.value = c.update.intervalMinutes;
  f.ssid.value = c.wifi.ssid; f.password.value = '';
  f.password.placeholder = c.wifi.passwordSet ? '•••••• (gespeichert)' : '';
  f.ntp.checked = c.ntp; f.portalSecure.checked = c.portalSecure; f.nopw.checked = false;
  syncVisibility();
}

function collect() {
  const c = {
    name: f.name.value.trim(), occasion: f.occasion.value.trim(),
    event: f.date.value ? f.date.value + 'T' + (f.time.value || '00:00') : '',
    tz: f.tzp.value || f.tz.value.trim(), layout: f.layout.value, showTime: f.showTime.checked,
    update: { mode: f.mode.value, intervalMinutes: Number(f.intervalMinutes.value) || 60 },
    wifi: { ssid: f.ssid.value }, ntp: f.ntp.checked, portalSecure: f.portalSecure.checked
  };
  if (f.password.value) c.wifi.password = f.password.value;
  else if (f.nopw.checked) c.wifi.password = '';
  return c;
}

function clearIssues() {
  document.querySelectorAll('.bad').forEach((e) => e.classList.remove('bad'));
  document.querySelectorAll('.hint').forEach((e) => e.remove());
}

function showIssues(issues) {
  for (const i of issues || []) {
    const el = f[FIELD[i.field] || i.field];
    const label = el && el.closest('label');
    if (!label) continue;
    label.classList.add('bad');
    const h = document.createElement('div');
    h.className = 'hint'; h.textContent = MSG[i.code] || i.code;
    label.appendChild(h);
  }
}

f.onsubmit = async (e) => {
  e.preventDefault(); clearIssues();
  try { fill(await api('config', collect())); toast('Gespeichert'); }
  catch (err) { showIssues(err.data && err.data.issues); toast('Bitte Eingaben prüfen', true); }
};
f.tzp.onchange = f.mode.onchange = syncVisibility;

function clock() {
  $('#clk').textContent = new Date().toLocaleString('de-DE');
}
$('#settime').onclick = async () => {
  try { await api('time', { epoch: Math.floor(Date.now() / 1000) }); toast('Uhrzeit übernommen'); }
  catch (e) { toast('Fehler: ' + e.message, true); }
};

async function status() {
  try {
    const s = await api('status');
    $('#dev').textContent = s.name;
    const rows = [
      ['Akku', s.batteryMv ? s.batteryPercent + ' % (' + (s.batteryMv / 1000).toFixed(2) + ' V)' : 'kein Akku'],
      ['Firmware', s.fw],
      ['Uhr', s.rtcOk ? (s.rtcLostPower ? 'Zeit verloren – bitte einstellen' : new Date(s.nowUtc * 1000).toLocaleString('de-DE')) : 'nicht gefunden'],
      ['Letzter Zeit-Sync', s.lastSync ? new Date(s.lastSync * 1000).toLocaleString('de-DE') : 'nie'],
      ['Einrichtung endet in', Math.ceil(s.setupRemainingS / 60) + ' min']
    ];
    $('#st').replaceChildren(...rows.flatMap(([k, v]) => {
      const dt = document.createElement('dt'); dt.textContent = k;
      const dd = document.createElement('dd'); dd.textContent = v;
      return [dt, dd];
    }));
  } catch (e) { toast('Status nicht verfügbar', true); }
}
$('#reload').onclick = status;

$('#ota').onclick = () => {
  const file = $('#fw').files[0];
  if (!file) return toast('Bitte Datei wählen', true);
  const code = prompt('Code vom Display eingeben (6 Ziffern):');
  if (!code) return;
  const fd = new FormData(); fd.append('file', file, file.name);
  const x = new XMLHttpRequest();
  x.open('POST', '/api/ota' + ($('#fs').checked ? '?type=fs' : ''));
  x.setRequestHeader('X-Setup-Code', code.trim());
  $('#prog').hidden = false;
  x.upload.onprogress = (e) => { if (e.lengthComputable) $('#prog').value = (100 * e.loaded) / e.total; };
  x.onload = () => toast(x.status === 200 ? 'Update installiert – Gerät startet neu'
    : x.status === 403 ? 'Falscher Code' : 'Update fehlgeschlagen', x.status !== 200);
  x.onerror = () => toast('Upload abgebrochen', true);
  x.send(fd);
};

$('#reset').onclick = async () => {
  if (!confirm('Wirklich alle Einstellungen löschen?')) return;
  const code = prompt('Code vom Display eingeben (6 Ziffern):');
  if (!code) return;
  try { await api('factory-reset', { confirm: 'RESET' }, code.trim()); toast('Zurückgesetzt – Gerät startet neu'); }
  catch (e) { toast(e.message === 'unauthorized' ? 'Falscher Code' : 'Fehler: ' + e.message, true); }
};

$('#done').onclick = async () => {
  try { await api('exit', {}); toast('Fertig! Das Display aktualisiert sich gleich.'); }
  catch (e) { toast('Fehler: ' + e.message, true); }
};

tabs(); clock(); setInterval(clock, 1000);
api('config').then(fill).catch(() => toast('Gerät nicht erreichbar', true));
status();

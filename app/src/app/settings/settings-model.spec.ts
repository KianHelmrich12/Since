import { applyJson, defaultConfig, toJson } from '../core/config/config';
import { readFixture } from '../core/config/fixtures.testing';
import { issueMessage } from '../core/messages';
import { CUSTOM_TZ, emptyForm, formKeyFor, fromApi, toPatch, validateForm } from './settings-model';

const full = () => applyJson(readFixture('valid', 'full.json'), defaultConfig()).config!;

describe('settings model', () => {
  it('round-trips the API config through the form', () => {
    const api = toJson(full(), false);
    const form = fromApi(api);
    expect(form.eventDate).toBe('2023-03-12');
    expect(form.eventTime).toBe('08:15');
    expect(form.tzPreset).toBe('CET-1CEST,M3.5.0,M10.5.0/3');
    expect(form.password).toBe('');
    const r = applyJson(toPatch(form), full());
    expect(r.status).toBe('ok');
    expect(toJson(r.config!, true)).toEqual(toJson(full(), true));
  });

  it('keeps the password when the field is empty and sends it when typed', () => {
    const form = fromApi(toJson(full(), false));
    expect(toPatch(form).wifi).toEqual({ ssid: 'Heimnetz' });
    form.password = 'neuesPasswort';
    expect(toPatch(form).wifi).toEqual({ ssid: 'Heimnetz', password: 'neuesPasswort' });
  });

  it('uses the free POSIX input for a custom zone', () => {
    const form = emptyForm();
    form.tzPreset = CUSTOM_TZ;
    form.tzCustom = ' <+0530>-5:30 ';
    expect(toPatch(form).tz).toBe('<+0530>-5:30');
    expect(validateForm(form)).toEqual([]);
    form.tzCustom = 'Europe/Berlin';
    expect(validateForm(form)).toEqual([{ field: 'tz', code: 'invalid' }]);
    expect(formKeyFor('tz', form)).toBe('tzCustom');
  });

  it('an empty date means no event', () => {
    const form = emptyForm();
    expect(toPatch(form).event).toBe('');
    form.eventDate = '2023-02-30';
    expect(validateForm(form)).toEqual([{ field: 'event', code: 'invalid' }]);
  });

  it('checks the interval only in interval mode', () => {
    const form = emptyForm();
    form.intervalMinutes = 3;
    expect(validateForm(form)).toEqual([]);
    form.updateMode = 'interval';
    expect(validateForm(form)).toEqual([{ field: 'update.intervalMinutes', code: 'out_of_range' }]);
    form.intervalMinutes = Number.NaN;
    expect(validateForm(form)).toEqual([{ field: 'update.intervalMinutes', code: 'out_of_range' }]);
  });

  it('reports ntp without wifi and short passwords with German messages', () => {
    const form = emptyForm();
    form.ntp = true;
    form.password = 'kurz';
    const issues = validateForm(form);
    expect(issues).toContainEqual({ field: 'ntp', code: 'requires_wifi' });
    expect(issues).toContainEqual({ field: 'wifi.password', code: 'length' });
    expect(issueMessage({ field: 'ntp', code: 'requires_wifi' })).toContain('SSID');
    expect(issueMessage({ field: 'wifi.password', code: 'length' })).toContain('8 bis 63');
  });
});

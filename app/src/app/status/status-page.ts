import { Component, computed, inject, signal } from '@angular/core';
import { Router } from '@angular/router';
import {
  ZAlert,
  ZBadge,
  ZButton,
  ZDialog,
  ZPageHeader,
  ZPanel,
  ZSpecItem,
  ZSpecList,
  ZToast,
} from 'zenit-ui';

import { DEFAULT_TZ } from '../core/config/tz';
import { DeviceService } from '../core/device.service';
import { formatDuration, formatEpochInTz, formatPercent, formatVolt } from '../core/format';
import { errorMessage } from '../core/messages';

@Component({
  selector: 'app-status-page',
  imports: [ZAlert, ZBadge, ZButton, ZPageHeader, ZPanel, ZSpecList],
  host: { class: 'z-stack' },
  templateUrl: './status-page.html',
})
export class StatusPage {
  protected readonly device = inject(DeviceService);
  private readonly toast = inject(ZToast);
  private readonly dialog = inject(ZDialog);
  private readonly router = inject(Router);

  protected readonly loading = signal(false);
  protected readonly ending = signal(false);
  protected readonly error = signal('');

  protected readonly items = computed<ZSpecItem[]>(() => {
    const s = this.device.status();
    if (!s) return [];
    const tz = this.device.config()?.tz ?? DEFAULT_TZ;
    const charging = s.charging === null ? 'unbekannt' : s.charging ? 'ja' : 'nein';
    return [
      { term: 'Name', value: s.name, mono: true },
      { term: 'Firmware', value: s.fw, mono: true },
      {
        term: 'Akku',
        value: formatPercent(s.batteryPercent),
        note: formatVolt(s.batteryMv),
        mono: true,
      },
      { term: 'Lädt', value: charging },
      { term: 'RTC', value: s.rtcOk ? 'erreichbar' : 'nicht erreichbar' },
      { term: 'RTC-Stromausfall', value: s.rtcLostPower ? 'ja, Uhrzeit neu stellen' : 'nein' },
      { term: 'Gerätezeit', value: formatEpochInTz(s.nowUtc, tz), mono: true },
      {
        term: 'Letzte Synchronisierung',
        value: s.lastSync === 0 ? 'noch nie' : formatEpochInTz(s.lastSync, tz),
        mono: s.lastSync !== 0,
      },
      { term: 'Setup endet in', value: formatDuration(s.setupRemainingS), mono: true },
    ];
  });

  constructor() {
    void this.refresh();
  }

  protected async refresh(): Promise<void> {
    this.loading.set(true);
    this.error.set('');
    try {
      await this.device.loadStatus();
    } catch (err) {
      this.error.set(errorMessage(err));
    } finally {
      this.loading.set(false);
    }
  }

  protected async neuZeichnen(): Promise<void> {
    try {
      await this.device.redraw();
      this.toast.info('Das Display zeichnet neu, sobald du das Setup beendest.');
    } catch (err) {
      this.error.set(errorMessage(err));
    }
  }

  protected setupBeenden(): void {
    this.dialog
      .confirm({
        title: 'Setup beenden?',
        body: 'Das Gerät schaltet Bluetooth und Hotspot aus und zeichnet das Display neu. Für weitere Änderungen startest du den Setup-Modus wieder mit KEY0.',
        confirmLabel: 'Setup beenden',
        cancelLabel: 'Abbrechen',
      })
      .subscribe((ok) => {
        if (ok) void this.beenden();
      });
  }

  private async beenden(): Promise<void> {
    this.ending.set(true);
    try {
      await this.device.exitSetup();
      this.toast.success('Setup beendet. Das Display zeichnet jetzt neu.');
      await this.router.navigateByUrl('/geraet');
    } catch (err) {
      this.error.set(errorMessage(err));
    } finally {
      this.ending.set(false);
    }
  }
}

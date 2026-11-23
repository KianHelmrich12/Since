import { Component, DestroyRef, computed, inject, signal } from '@angular/core';
import { ZAlert, ZButton, ZPageHeader, ZPanel, ZSpecItem, ZSpecList, ZToast } from 'zenit-ui';

import { DEFAULT_TZ } from '../core/config/tz';
import { DeviceService } from '../core/device.service';
import { formatEpochInTz, formatOffset } from '../core/format';
import { errorMessage } from '../core/messages';

@Component({
  selector: 'app-time-page',
  imports: [ZAlert, ZButton, ZPageHeader, ZPanel, ZSpecList],
  host: { class: 'z-stack' },
  templateUrl: './time-page.html',
})
export class TimePage {
  protected readonly device = inject(DeviceService);
  private readonly toast = inject(ZToast);

  protected readonly nowMs = signal(Date.now());
  protected readonly sending = signal(false);
  protected readonly error = signal('');

  private readonly tz = computed(() => this.device.config()?.tz ?? DEFAULT_TZ);

  /** Gerätezeit, aus dem letzten Status hochgerechnet */
  private readonly deviceEpoch = computed(() => {
    const s = this.device.status();
    if (!s) return null;
    return s.nowUtc + Math.floor((this.nowMs() - this.device.statusAt()) / 1000);
  });

  protected readonly items = computed<ZSpecItem[]>(() => {
    const phone = Math.floor(this.nowMs() / 1000);
    const dev = this.deviceEpoch();
    const items: ZSpecItem[] = [
      { term: 'Handy', value: formatEpochInTz(phone, this.tz(), true), mono: true },
      {
        term: 'Gerät',
        value: dev === null ? 'wird gelesen' : formatEpochInTz(dev, this.tz(), true),
        mono: dev !== null,
      },
    ];
    if (dev !== null)
      items.push({ term: 'Abweichung', value: `Gerät geht ${formatOffset(dev - phone)}` });
    items.push({ term: 'Zeitzone', value: this.tz(), mono: true });
    return items;
  });

  constructor() {
    const timer = setInterval(() => this.nowMs.set(Date.now()), 1000);
    inject(DestroyRef).onDestroy(() => clearInterval(timer));
    void this.refresh();
  }

  protected async refresh(): Promise<void> {
    this.error.set('');
    try {
      await this.device.loadStatus();
    } catch (err) {
      this.error.set(errorMessage(err));
    }
  }

  protected async uebernehmen(): Promise<void> {
    this.sending.set(true);
    this.error.set('');
    try {
      await this.device.setTimeFromPhone();
      await this.device.loadStatus();
      this.toast.success('Uhrzeit übernommen.');
    } catch (err) {
      this.error.set(errorMessage(err));
    } finally {
      this.sending.set(false);
    }
  }
}

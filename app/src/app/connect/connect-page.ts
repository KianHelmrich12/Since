import { Component, computed, inject, signal } from '@angular/core';
import { RouterLink } from '@angular/router';
import {
  ZAlert,
  ZButton,
  ZEmptyAction,
  ZEmptyState,
  ZPageHeader,
  ZPanel,
  ZPanelActions,
  ZRow,
  ZRowMain,
  ZRows,
  ZSpecItem,
  ZSpecList,
  ZToast,
} from 'zenit-ui';

import { DeviceService } from '../core/device.service';
import { errorMessage } from '../core/messages';
import { FoundDevice } from '../core/transport/device-transport';
import { NBSP } from '../core/format';

@Component({
  selector: 'app-connect-page',
  imports: [
    RouterLink,
    ZAlert,
    ZButton,
    ZEmptyAction,
    ZEmptyState,
    ZPageHeader,
    ZPanel,
    ZPanelActions,
    ZRow,
    ZRowMain,
    ZRows,
    ZSpecList,
  ],
  host: { class: 'z-stack' },
  templateUrl: './connect-page.html',
})
export class ConnectPage {
  protected readonly device = inject(DeviceService);
  private readonly toast = inject(ZToast);

  protected readonly connectingId = signal<string | null>(null);
  protected readonly error = signal('');
  protected readonly searched = signal(false);

  protected readonly infoItems = computed<ZSpecItem[]>(() => {
    const info = this.device.info();
    const dev = this.device.device();
    if (!info || !dev) return [];
    return [
      { term: 'Name', value: info.name, mono: true },
      { term: 'Firmware', value: info.fw, mono: true },
      { term: 'Protokoll', value: `Version ${info.proto}` },
    ];
  });

  protected meta(d: FoundDevice): string {
    return d.rssi === undefined ? d.id : `${d.id} · ${d.rssi}${NBSP}dBm`;
  }

  protected async scan(): Promise<void> {
    this.error.set('');
    this.searched.set(true);
    try {
      await this.device.scan();
    } catch (err) {
      this.error.set(errorMessage(err));
    }
  }

  protected async connect(d: FoundDevice): Promise<void> {
    this.error.set('');
    await this.device.stopScan();
    this.connectingId.set(d.id);
    try {
      await this.device.connect(d);
      this.toast.success(`Mit ${d.name} verbunden.`);
    } catch (err) {
      this.error.set(errorMessage(err));
    } finally {
      this.connectingId.set(null);
    }
  }

  protected async disconnect(): Promise<void> {
    await this.device.disconnect();
    this.toast.info('Verbindung getrennt.');
  }
}

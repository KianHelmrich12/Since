import { Component, effect, inject } from '@angular/core';
import { Router, RouterLink, RouterLinkActive, RouterOutlet } from '@angular/router';
import {
  ZAppHeader,
  ZBadge,
  ZBrand,
  ZHeaderEnd,
  ZHeaderLink,
  ZSkipLink,
  ZToast,
  ZToastOutlet,
} from 'zenit-ui';

import { DeviceService } from './core/device.service';

interface NavLink {
  route: string;
  name: string;
  needsDevice: boolean;
}

@Component({
  imports: [
    RouterOutlet,
    RouterLink,
    RouterLinkActive,
    ZAppHeader,
    ZBadge,
    ZBrand,
    ZHeaderEnd,
    ZHeaderLink,
    ZSkipLink,
    ZToastOutlet,
  ],
  selector: 'app-root',
  templateUrl: './app.html',
})
export class App {
  protected readonly device = inject(DeviceService);
  private readonly router = inject(Router);
  private readonly toast = inject(ZToast);

  protected readonly links: readonly NavLink[] = [
    { route: '/geraet', name: 'Gerät', needsDevice: false },
    { route: '/einstellungen', name: 'Einstellungen', needsDevice: true },
    { route: '/uhrzeit', name: 'Uhrzeit', needsDevice: true },
    { route: '/status', name: 'Status', needsDevice: true },
  ];

  constructor() {
    // Bricht die Verbindung ab (Setup-Timeout, außer Reichweite), zurück zur Gerätesuche.
    effect(() => {
      if (this.device.lostConnection()) {
        this.toast.warning(
          'Verbindung zum Gerät getrennt. Starte den Setup-Modus und verbinde neu.',
        );
        void this.router.navigateByUrl('/geraet');
      }
    });
  }
}

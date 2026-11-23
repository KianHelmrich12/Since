import { inject } from '@angular/core';
import { CanActivateFn, Router, Routes } from '@angular/router';

import { DeviceService } from './core/device.service';

/** Seiten, die ein verbundenes Gerät brauchen, leiten sonst zur Gerätesuche. */
const requireDevice: CanActivateFn = () =>
  inject(DeviceService).connected() || inject(Router).createUrlTree(['/geraet']);

export const routes: Routes = [
  { path: '', pathMatch: 'full', redirectTo: 'geraet' },
  {
    path: 'geraet',
    title: 'Gerät · since',
    loadComponent: () => import('./connect/connect-page').then((m) => m.ConnectPage),
  },
  {
    path: 'einstellungen',
    title: 'Einstellungen · since',
    canActivate: [requireDevice],
    loadComponent: () => import('./settings/settings-page').then((m) => m.SettingsPage),
  },
  {
    path: 'uhrzeit',
    title: 'Uhrzeit · since',
    canActivate: [requireDevice],
    loadComponent: () => import('./time/time-page').then((m) => m.TimePage),
  },
  {
    path: 'status',
    title: 'Status · since',
    canActivate: [requireDevice],
    loadComponent: () => import('./status/status-page').then((m) => m.StatusPage),
  },
  { path: '**', redirectTo: 'geraet' },
];

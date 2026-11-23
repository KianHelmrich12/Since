import { ApplicationConfig, provideBrowserGlobalErrorListeners } from '@angular/core';
import { provideRouter, withComponentInputBinding } from '@angular/router';
import { provideZenitTheme } from 'zenit-ui';

import { routes } from './app.routes';
import { provideDeviceTransport } from './core/transport/provide-transport';

export const appConfig: ApplicationConfig = {
  providers: [
    provideBrowserGlobalErrorListeners(),
    provideRouter(routes, withComponentInputBinding()),
    provideZenitTheme(),
    provideDeviceTransport(),
  ],
};

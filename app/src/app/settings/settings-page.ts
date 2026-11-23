import { Component, computed, inject, signal } from '@angular/core';
import {
  FormField,
  PathKind,
  SchemaPath,
  SchemaPathRules,
  form,
  submit,
  validate,
} from '@angular/forms/signals';
import {
  ZAlert,
  ZButton,
  ZCheckbox,
  ZField,
  ZInput,
  ZPanel,
  ZPageHeader,
  ZSegment,
  ZSegmentOption,
  ZSelect,
  ZSetting,
  ZToast,
} from 'zenit-ui';

import { Config, defaultConfig, ValidationIssue } from '../core/config/config';
import { DeviceService } from '../core/device.service';
import { errorMessage, issueMessage } from '../core/messages';
import { DeviceError } from '../core/protocol/protocol';
import {
  CUSTOM_TZ,
  FormKey,
  SettingsForm,
  TZ_PRESETS,
  configFromApi,
  emptyForm,
  formKeyFor,
  fromApi,
  toPatch,
  validateForm,
} from './settings-model';

interface FieldLike {
  touched(): boolean;
  errors(): readonly { message?: string }[];
}

const FORM_KEYS: readonly FormKey[] = [
  'name',
  'occasion',
  'eventDate',
  'eventTime',
  'tzPreset',
  'tzCustom',
  'layout',
  'showTime',
  'updateMode',
  'intervalMinutes',
  'ssid',
  'password',
  'ntp',
  'portalSecure',
];

@Component({
  selector: 'app-settings-page',
  imports: [
    FormField,
    ZAlert,
    ZButton,
    ZCheckbox,
    ZField,
    ZInput,
    ZPageHeader,
    ZPanel,
    ZSegment,
    ZSelect,
    ZSetting,
  ],
  host: { class: 'z-stack' },
  templateUrl: './settings-page.html',
})
export class SettingsPage {
  protected readonly device = inject(DeviceService);
  private readonly toast = inject(ZToast);

  protected readonly tzPresets = TZ_PRESETS;
  protected readonly customTz = CUSTOM_TZ;
  protected readonly layouts: ZSegmentOption[] = [
    { value: 'standard', label: 'Standard' },
    { value: 'minimal', label: 'Minimal' },
  ];
  protected readonly updateModes: ZSegmentOption[] = [
    { value: 'auto', label: 'Automatisch' },
    { value: 'interval', label: 'Intervall' },
  ];

  /** Die zuletzt vom Gerät gelesene Config, gegen die das Formular geprüft wird. */
  private readonly base = signal<Config>(defaultConfig());
  protected readonly loading = signal(true);
  protected readonly saving = signal(false);
  protected readonly error = signal('');
  /** Fehler, die das Gerät gemeldet hat, je Formularfeld */
  private readonly deviceIssues = signal<Partial<Record<FormKey, string>>>({});

  protected readonly model = signal<SettingsForm>(emptyForm());
  protected readonly formular = form(this.model, (p) => {
    for (const key of FORM_KEYS) {
      // Jedes Feld bekommt die Fehler, die die Gerätelogik ihm zuordnet.
      const path = p[key] as unknown as SchemaPath<
        unknown,
        SchemaPathRules.Supported,
        PathKind.Child
      >;
      validate(path, ({ valueOf }) => {
        const values = valueOf(p);
        return validateForm(values, this.base())
          .filter((i) => formKeyFor(i.field, values) === key)
          .map((i) => ({ kind: i.code, message: issueMessage(i) }));
      });
    }
  });

  protected readonly passwordSet = computed(() => this.device.config()?.wifi.passwordSet === true);

  constructor() {
    void this.load();
  }

  /** Erste Fehlermeldung eines Felds: aus der App, sobald es berührt ist, sonst vom Gerät. */
  protected fehler(key: FormKey, feld: FieldLike): string {
    if (feld.touched()) {
      const msg = feld.errors()[0]?.message;
      if (msg) return msg;
    }
    return this.deviceIssues()[key] ?? '';
  }

  protected async load(): Promise<void> {
    this.loading.set(true);
    this.error.set('');
    try {
      this.apply(await this.device.loadConfig());
    } catch (err) {
      this.error.set(errorMessage(err));
    } finally {
      this.loading.set(false);
    }
  }

  protected speichern(): void {
    void submit(this.formular, async () => {
      this.saving.set(true);
      this.error.set('');
      this.deviceIssues.set({});
      try {
        const cfg = await this.device.saveConfig(toPatch(this.model()));
        this.apply(cfg);
        this.toast.success(
          'Einstellungen gespeichert. Das Display zeichnet neu, wenn du das Setup beendest.',
        );
      } catch (err) {
        this.showDeviceError(err);
      } finally {
        this.saving.set(false);
      }
    });
  }

  protected async passwortEntfernen(): Promise<void> {
    this.error.set('');
    try {
      this.apply(await this.device.saveConfig({ wifi: { password: '' } }));
      this.toast.success('WLAN-Passwort entfernt.');
    } catch (err) {
      this.showDeviceError(err);
    }
  }

  private apply(cfg: NonNullable<ReturnType<DeviceService['config']>>): void {
    this.base.set(configFromApi(cfg));
    this.model.set(fromApi(cfg));
    this.formular().reset();
  }

  private showDeviceError(err: unknown): void {
    this.error.set(errorMessage(err));
    if (err instanceof DeviceError && err.issues.length > 0) {
      const byKey: Partial<Record<FormKey, string>> = {};
      for (const issue of err.issues as ValidationIssue[]) {
        const key = formKeyFor(issue.field, this.model());
        if (key && !byKey[key]) byKey[key] = issueMessage(issue);
      }
      this.deviceIssues.set(byKey);
    }
  }
}

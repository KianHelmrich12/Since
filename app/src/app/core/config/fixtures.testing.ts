// Nur für Tests: liest die gemeinsamen Fixtures aus ../shared/fixtures/config.
// Die Tests laufen unter Vitest in Node, deshalb ist node:fs hier verfügbar.
import { readdirSync, readFileSync } from 'node:fs';
import { join, resolve } from 'node:path';

export const FIXTURES_DIR = resolve(process.cwd(), '..', 'shared', 'fixtures', 'config');

export interface Fixture {
  name: string;
  text: string;
}

export function loadFixtures(kind: 'valid' | 'invalid'): Fixture[] {
  const dir = join(FIXTURES_DIR, kind);
  return readdirSync(dir)
    .filter((n) => n.endsWith('.json'))
    .sort()
    .map((name) => ({ name, text: readFileSync(join(dir, name), 'utf8') }));
}

export function readFixture(kind: 'valid' | 'invalid', name: string): string {
  return readFileSync(join(FIXTURES_DIR, kind, name), 'utf8');
}

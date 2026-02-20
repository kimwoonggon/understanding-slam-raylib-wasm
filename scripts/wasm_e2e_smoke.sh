#!/usr/bin/env bash
set -euo pipefail

ROOT_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")/.." && pwd)"
BUILD_DIR="${BUILD_DIR:-${ROOT_DIR}/build-wasm}"
PORT="${PORT:-8090}"

HTML_FILE="${BUILD_DIR}/slam-raylib.html"
if [[ ! -f "${HTML_FILE}" ]]; then
  echo "[ERROR] Missing ${HTML_FILE}. Build wasm first: ./scripts/build_wasm.sh" >&2
  exit 1
fi

if ! command -v node >/dev/null 2>&1; then
  echo "[ERROR] node is required for playwright-based wasm e2e." >&2
  exit 1
fi

if ! command -v npx >/dev/null 2>&1; then
  echo "[ERROR] npx is required for playwright-based wasm e2e." >&2
  exit 1
fi

if ! npx playwright --version >/dev/null 2>&1; then
  cat >&2 <<'MSG'
[ERROR] Playwright is not available.
Install once in this repository:
  npm init -y
  npm i -D playwright
MSG
  exit 1
fi

echo "[INFO] Serving ${BUILD_DIR} on http://127.0.0.1:${PORT}"
python3 -m http.server --directory "${BUILD_DIR}" "${PORT}" >/tmp/slam_wasm_server.log 2>&1 &
SERVER_PID=$!
trap 'kill ${SERVER_PID} >/dev/null 2>&1 || true' EXIT

node <<'NODE'
const { chromium } = require('playwright');

(async () => {
  const port = process.env.PORT || '8090';
  const url = `http://127.0.0.1:${port}/slam-raylib.html`;
  const browser = await chromium.launch({ headless: true });
  const page = await browser.newPage();
  const errors = [];
  page.on('pageerror', (e) => errors.push(String(e)));
  page.on('console', (msg) => {
    if (msg.type() === 'error') errors.push(msg.text());
  });

  await page.goto(url, { waitUntil: 'networkidle' });
  await page.waitForTimeout(2000);
  const hasCanvas = await page.locator('canvas').count();

  await browser.close();

  if (hasCanvas < 1) {
    console.error('[ERROR] No canvas element detected.');
    process.exit(2);
  }
  if (errors.length > 0) {
    console.error('[ERROR] Browser/runtime errors detected:');
    for (const e of errors) console.error(` - ${e}`);
    process.exit(3);
  }
  console.log('[OK] wasm e2e smoke passed (canvas visible, no runtime errors).');
})().catch((err) => {
  console.error(err);
  process.exit(4);
});
NODE

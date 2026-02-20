#!/usr/bin/env bash
set -euo pipefail

ROOT_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")/.." && pwd)"
BUILD_DIR="${BUILD_DIR:-${ROOT_DIR}/build-wasm}"
PORT="${PORT:-8090}"
SERVER_BUILD_DIR="${SERVER_BUILD_DIR:-${ROOT_DIR}/build-release}"
SERVER_EXE="${SERVER_EXE:-${SERVER_BUILD_DIR}/slam-static-server}"

HTML_FILE="${BUILD_DIR}/slam-raylib.html"
if [[ ! -f "${HTML_FILE}" ]]; then
  echo "[ERROR] Missing ${HTML_FILE}. Build wasm first: ./scripts/build_wasm.sh" >&2
  exit 1
fi

if [[ ! -x "${SERVER_EXE}" ]]; then
  echo "[INFO] Building C++ static server (${SERVER_EXE})"
  cmake -S "${ROOT_DIR}" -B "${SERVER_BUILD_DIR}" -DCMAKE_BUILD_TYPE=Release >/dev/null
  cmake --build "${SERVER_BUILD_DIR}" --target slam-static-server -j >/dev/null
fi

if ! command -v node >/dev/null 2>&1; then
  echo "[ERROR] node is required for playwright-based wasm e2e." >&2
  exit 1
fi

if ! command -v npx >/dev/null 2>&1; then
  echo "[ERROR] npx is required for playwright-based wasm e2e." >&2
  exit 1
fi

if ! node -e "require('playwright')" >/dev/null 2>&1; then
  echo "[INFO] Installing local playwright dependency"
  if [[ ! -f "${ROOT_DIR}/package.json" ]]; then
    npm init -y >/dev/null
  fi
  npm i -D playwright >/dev/null
fi

echo "[INFO] Ensuring Playwright Chromium is installed"
npx playwright install chromium >/dev/null

echo "[INFO] Serving ${BUILD_DIR} on http://127.0.0.1:${PORT}"
"${SERVER_EXE}" --root "${BUILD_DIR}" --port "${PORT}" >/tmp/slam_wasm_server.log 2>&1 &
SERVER_PID=$!
trap 'kill ${SERVER_PID} >/dev/null 2>&1 || true' EXIT

node <<'NODE'
const { chromium } = require('playwright');

(async () => {
  const port = process.env.PORT || '8090';
  const url = `http://127.0.0.1:${port}/slam-raylib.html`;
  const browser = await chromium.launch({
    headless: true,
    args: [
      '--use-angle=swiftshader',
      '--use-gl=angle',
      '--enable-webgl',
      '--ignore-gpu-blocklist',
      '--disable-gpu-sandbox'
    ],
  });
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

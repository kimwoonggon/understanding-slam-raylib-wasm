#!/usr/bin/env bash
set -euo pipefail

ROOT_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")/.." && pwd)"
BUILD_DIR="${BUILD_DIR:-${ROOT_DIR}/build-wasm}"
PORT="${PORT:-8090}"
HOST="${HOST:-127.0.0.1}"
SERVER_BUILD_DIR="${SERVER_BUILD_DIR:-${ROOT_DIR}/build-release}"
SERVER_EXE="${SERVER_EXE:-${SERVER_BUILD_DIR}/slam-static-server}"
SAMPLE_SECONDS="${SAMPLE_SECONDS:-20}"

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
  echo "[ERROR] node is required for playwright-based telemetry." >&2
  exit 1
fi

if ! command -v npx >/dev/null 2>&1; then
  echo "[ERROR] npx is required for playwright-based telemetry." >&2
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

echo "[INFO] Serving ${BUILD_DIR} on http://${HOST}:${PORT}"
"${SERVER_EXE}" --root "${BUILD_DIR}" --host "${HOST}" --port "${PORT}" >/tmp/slam_wasm_fps_server.log 2>&1 &
SERVER_PID=$!
trap 'kill ${SERVER_PID} >/dev/null 2>&1 || true' EXIT

HOST="${HOST}" PORT="${PORT}" SAMPLE_SECONDS="${SAMPLE_SECONDS}" node <<'NODE'
const { chromium } = require('playwright');

(async () => {
  const host = process.env.HOST || '127.0.0.1';
  const port = process.env.PORT || '8090';
  const sampleSeconds = Number.parseInt(process.env.SAMPLE_SECONDS || '20', 10);
  const url = `http://${host}:${port}/slam-raylib.html`;
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

  const canvas = page.locator('canvas');
  if ((await canvas.count()) < 1) {
    throw new Error('canvas not found');
  }
  await canvas.click();
  await page.waitForTimeout(100);

  await page.waitForFunction(
    () =>
      typeof window !== 'undefined' &&
      !!window.__slamDebug &&
      Number.isFinite(window.__slamDebug.poseX) &&
      Number.isFinite(window.__slamDebug.poseY) &&
      Number.isFinite(window.__slamDebug.fps),
    { timeout: 8000 },
  );

  const readDebug = async () =>
    page.evaluate(() => ({
      poseX: window.__slamDebug.poseX,
      poseY: window.__slamDebug.poseY,
      fps: window.__slamDebug.fps,
      hitHistorySize: window.__slamDebug.hitHistorySize,
      accumulateHits: !!window.__slamDebug.accumulateHits,
    }));

  const box = await canvas.boundingBox();
  if (!box) {
    throw new Error('canvas bounding box missing');
  }

  const clamp = (value, minValue, maxValue) => Math.max(minValue, Math.min(maxValue, value));
  const toViewport = (x, y) => ({
    x: box.x + x * (box.width / 960),
    y: box.y + y * (box.height / 640),
  });

  const waitForAccumulate = async (timeoutMs) => {
    const start = Date.now();
    while (Date.now() - start < timeoutMs) {
      const sample = await readDebug();
      if (sample.accumulateHits) {
        return sample;
      }
      await page.waitForTimeout(80);
    }
    return null;
  };

  let debug = await readDebug();
  if (!debug || !debug.accumulateHits) {
    await canvas.click();
    await page.keyboard.down('g');
    await page.waitForTimeout(220);
    await page.keyboard.up('g');
    debug = await waitForAccumulate(1500);
  }
  if (!debug || !debug.accumulateHits) {
    await page.keyboard.down('g');
    await page.waitForTimeout(220);
    await page.keyboard.up('g');
    debug = await waitForAccumulate(1500);
  }
  if (!debug || !debug.accumulateHits) {
    throw new Error('failed to enable accumulate mode');
  }

  const rng = (() => {
    let state = 0x12345678;
    return () => {
      state ^= state << 13;
      state ^= state >>> 17;
      state ^= state << 5;
      return ((state >>> 0) % 1000000) / 1000000;
    };
  })();

  const fpsSamples = [];
  const hitSamples = [];

  const sampleCount = Math.max(10, sampleSeconds * 10);
  for (let i = 0; i < sampleCount; ++i) {
    if (i % 3 === 0) {
      const fromX = clamp(Math.round(30 + rng() * 720), 20, 760);
      const fromY = clamp(Math.round(30 + rng() * 560), 20, 620);
      const toX = clamp(Math.round(30 + rng() * 720), 20, 760);
      const toY = clamp(Math.round(30 + rng() * 560), 20, 620);
      const start = toViewport(fromX, fromY);
      const end = toViewport(toX, toY);
      await page.mouse.move(start.x, start.y);
      await page.mouse.down();
      await page.mouse.move(end.x, end.y, { steps: 10 });
      await page.mouse.up();
    }

    const sample = await readDebug();
    if (Number.isFinite(sample.fps) && sample.fps >= 0) {
      fpsSamples.push(sample.fps);
    }
    if (Number.isFinite(sample.hitHistorySize) && sample.hitHistorySize >= 0) {
      hitSamples.push(sample.hitHistorySize);
    }
    await page.waitForTimeout(100);
  }

  if (errors.length > 0) {
    throw new Error(`console/page errors detected: ${errors.join(' | ')}`);
  }
  if (fpsSamples.length < 10) {
    throw new Error(`insufficient fps samples: ${fpsSamples.length}`);
  }
  if (hitSamples.length < 10) {
    throw new Error(`insufficient hit samples: ${hitSamples.length}`);
  }

  const fpsSorted = [...fpsSamples].sort((a, b) => a - b);
  const sum = fpsSamples.reduce((acc, value) => acc + value, 0);
  const avg = sum / fpsSamples.length;
  const min = fpsSorted[0];
  const max = fpsSorted[fpsSorted.length - 1];
  const p50 = fpsSorted[Math.floor(0.50 * (fpsSorted.length - 1))];
  const p95 = fpsSorted[Math.floor(0.95 * (fpsSorted.length - 1))];

  const hitStart = hitSamples[0];
  const hitEnd = hitSamples[hitSamples.length - 1];
  const hitMax = Math.max(...hitSamples);
  const hitGrowth = hitEnd - hitStart;

  console.log(
    `[METRIC] wasm_fps samples=${fpsSamples.length} min=${min.toFixed(2)} p50=${p50.toFixed(2)} p95=${p95.toFixed(2)} avg=${avg.toFixed(2)} max=${max.toFixed(2)}`
  );
  console.log(
    `[METRIC] wasm_hits samples=${hitSamples.length} start=${hitStart} end=${hitEnd} max=${hitMax} growth=${hitGrowth}`
  );

  if (hitMax < 200) {
    throw new Error(`insufficient accumulate coverage: max hitHistorySize=${hitMax}`);
  }

  await browser.close();
})();
NODE

echo "[OK] wasm FPS telemetry captured."

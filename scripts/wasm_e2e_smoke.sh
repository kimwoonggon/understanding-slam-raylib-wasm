#!/usr/bin/env bash
set -euo pipefail

ROOT_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")/.." && pwd)"
BUILD_DIR="${BUILD_DIR:-${ROOT_DIR}/build-wasm}"
PORT="${PORT:-8090}"
HOST="${HOST:-127.0.0.1}"
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

echo "[INFO] Serving ${BUILD_DIR} on http://${HOST}:${PORT}"
"${SERVER_EXE}" --root "${BUILD_DIR}" --host "${HOST}" --port "${PORT}" >/tmp/slam_wasm_server.log 2>&1 &
SERVER_PID=$!
trap 'kill ${SERVER_PID} >/dev/null 2>&1 || true' EXIT

node <<'NODE'
const { chromium } = require('playwright');

(async () => {
  const host = process.env.HOST || '127.0.0.1';
  const port = process.env.PORT || '8090';
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
  const hasCanvas = await page.locator('canvas').count();
  const canvasBox = hasCanvas > 0 ? await page.locator('canvas').boundingBox() : null;

  if (hasCanvas > 0) {
    await page.click('canvas');
  }

  await page.waitForFunction(
    () =>
      typeof window !== 'undefined' &&
      !!window.__slamDebug &&
      Number.isFinite(window.__slamDebug.poseX) &&
      Number.isFinite(window.__slamDebug.poseY),
    { timeout: 5000 },
  );

  const readDebug = async () => {
    return page.evaluate(() => ({
      poseX: window.__slamDebug.poseX,
      poseY: window.__slamDebug.poseY,
      keyboardIntent: !!window.__slamDebug.keyboardIntent,
      audioEnabled: !!window.__slamDebug.audioEnabled,
      mazeAssetPresent: !!window.__slamDebug.mazeAssetPresent,
      scanAssetPresent: !!window.__slamDebug.scanAssetPresent,
      collisionAssetPresent: !!window.__slamDebug.collisionAssetPresent,
      scanSoundReady: !!window.__slamDebug.scanSoundReady,
      collisionSoundReady: !!window.__slamDebug.collisionSoundReady,
    }));
  };

  const initialDebug = await readDebug();

  const distance = (a, b) => Math.hypot(b.poseX - a.poseX, b.poseY - a.poseY);

  const probeKey = async (key) => {
    const start = await readDebug();
    await page.keyboard.down(key);
    await page.waitForTimeout(220);
    const end = await readDebug();
    await page.keyboard.up(key);
    await page.waitForTimeout(80);
    return { key, delta: distance(start, end) };
  };

  const probeResults = [];
  for (const key of ['w', 'a', 's', 'd']) {
    probeResults.push(await probeKey(key));
  }
  probeResults.sort((lhs, rhs) => rhs.delta - lhs.delta);
  const best = probeResults[0];

  const holdStart = await readDebug();
  const intentSamples = [];
  await page.keyboard.down(best.key);
  for (let i = 0; i < 8; ++i) {
    await page.waitForTimeout(100);
    const sample = await readDebug();
    intentSamples.push(sample.keyboardIntent);
  }
  const holdEnd = await readDebug();
  await page.keyboard.up(best.key);
  await page.waitForTimeout(180);
  const afterRelease = await readDebug();

  const holdDelta = distance(holdStart, holdEnd);
  const intentTrueRatio =
    intentSamples.length > 0
      ? intentSamples.filter(Boolean).length / intentSamples.length
      : 0.0;
  const postKeyDebug = await readDebug();

  const clamp = (value, minValue, maxValue) =>
    Math.max(minValue, Math.min(maxValue, value));
  const mapXMax = 760;
  const mapYMax = 620;
  const canvasWidth = 960;
  const canvasHeight = 640;
  const toViewport = (canvasX, canvasY) => {
    if (!canvasBox) {
      return { x: canvasX, y: canvasY };
    }
    const scaleX = canvasBox.width / canvasWidth;
    const scaleY = canvasBox.height / canvasHeight;
    return {
      x: canvasBox.x + canvasX * scaleX,
      y: canvasBox.y + canvasY * scaleY,
    };
  };
  const dragVectors = [
    { dx: 80, dy: 0 },
    { dx: -80, dy: 0 },
    { dx: 0, dy: 80 },
    { dx: 0, dy: -80 },
    { dx: 64, dy: 48 },
    { dx: -64, dy: 48 },
  ];

  const performDragAttempt = async (vector) => {
    const start = await readDebug();
    const fromX = clamp(Math.round(start.poseX * 8), 20, mapXMax);
    const fromY = clamp(Math.round(start.poseY * 8), 20, mapYMax);
    const toX = clamp(fromX + vector.dx, 20, mapXMax);
    const toY = clamp(fromY + vector.dy, 20, mapYMax);
    const fromViewport = toViewport(fromX, fromY);
    const toViewportPoint = toViewport(toX, toY);

    await page.mouse.move(fromViewport.x, fromViewport.y);
    await page.mouse.down();
    await page.waitForTimeout(120);
    await page.mouse.move(toViewportPoint.x, toViewportPoint.y, { steps: 12 });
    await page.waitForTimeout(120);
    await page.mouse.up();
    await page.waitForTimeout(120);

    const end = await readDebug();
    return {
      fromX,
      fromY,
      toX,
      toY,
      delta: distance(start, end),
    };
  };

  const attemptDragMovement = async () => {
    let best = { delta: 0.0 };
    for (const vector of dragVectors) {
      const result = await performDragAttempt(vector);
      if (result.delta > best.delta) {
        best = result;
      }
      if (best.delta >= 0.75) {
        break;
      }
    }
    return best;
  };

  const baselineDrag = await attemptDragMovement();
  const controlButtons = [
    { name: 'RESET', x: 870, y: 28 },
    { name: 'WORLD', x: 870, y: 72 },
    { name: 'GREEN', x: 870, y: 116 },
  ];
  const controlDragResults = [];

  for (const button of controlButtons) {
    const buttonViewport = toViewport(button.x, button.y);
    await page.mouse.click(buttonViewport.x, buttonViewport.y);
    await page.waitForTimeout(140);
    const result = await attemptDragMovement();
    controlDragResults.push({ button: button.name, delta: result.delta });
  }

  const touchApiReady = await page.evaluate(
    () => typeof Touch !== 'undefined' && typeof TouchEvent !== 'undefined',
  );

  const dispatchTouchTap = async (canvasX, canvasY, identifier) => {
    const point = toViewport(canvasX, canvasY);
    await page.evaluate(({ point, identifier }) => {
      const canvas = document.getElementById('canvas');
      if (!canvas) {
        return;
      }
      const touch = new Touch({
        identifier,
        target: canvas,
        clientX: point.x,
        clientY: point.y,
        pageX: point.x,
        pageY: point.y,
        screenX: point.x,
        screenY: point.y,
        radiusX: 2,
        radiusY: 2,
        rotationAngle: 0,
        force: 1,
      });
      const startEvent = new TouchEvent('touchstart', {
        bubbles: true,
        cancelable: true,
        touches: [touch],
        targetTouches: [touch],
        changedTouches: [touch],
      });
      const endEvent = new TouchEvent('touchend', {
        bubbles: true,
        cancelable: true,
        touches: [],
        targetTouches: [],
        changedTouches: [touch],
      });
      canvas.dispatchEvent(startEvent);
      canvas.dispatchEvent(endEvent);
    }, { point, identifier });
  };

  const dispatchTouchDrag = async (fromCanvasX, fromCanvasY, toCanvasX, toCanvasY, identifier) => {
    const from = toViewport(fromCanvasX, fromCanvasY);
    const to = toViewport(toCanvasX, toCanvasY);
    await page.evaluate(async ({ from, to, identifier }) => {
      const canvas = document.getElementById('canvas');
      if (!canvas) {
        return;
      }
      const makeTouch = (x, y) =>
        new Touch({
          identifier,
          target: canvas,
          clientX: x,
          clientY: y,
          pageX: x,
          pageY: y,
          screenX: x,
          screenY: y,
          radiusX: 2,
          radiusY: 2,
          rotationAngle: 0,
          force: 1,
        });
      const dispatch = (type, x, y) => {
        const touch = makeTouch(x, y);
        const event = new TouchEvent(type, {
          bubbles: true,
          cancelable: true,
          touches: type === 'touchend' ? [] : [touch],
          targetTouches: type === 'touchend' ? [] : [touch],
          changedTouches: [touch],
        });
        canvas.dispatchEvent(event);
      };

      dispatch('touchstart', from.x, from.y);
      const steps = 8;
      for (let i = 1; i <= steps; i += 1) {
        const t = i / steps;
        const x = from.x + (to.x - from.x) * t;
        const y = from.y + (to.y - from.y) * t;
        dispatch('touchmove', x, y);
        await new Promise((resolve) => setTimeout(resolve, 20));
      }
      dispatch('touchend', to.x, to.y);
    }, { from, to, identifier });
    await page.waitForTimeout(120);
  };

  const performTouchDragAttempt = async (vector, identifier) => {
    const start = await readDebug();
    const fromX = clamp(Math.round(start.poseX * 8), 20, mapXMax);
    const fromY = clamp(Math.round(start.poseY * 8), 20, mapYMax);
    const toX = clamp(fromX + vector.dx, 20, mapXMax);
    const toY = clamp(fromY + vector.dy, 20, mapYMax);
    await dispatchTouchDrag(fromX, fromY, toX, toY, identifier);
    const end = await readDebug();
    return {
      fromX,
      fromY,
      toX,
      toY,
      delta: distance(start, end),
    };
  };

  const attemptTouchMovement = async (identifierStart = 200) => {
    let best = { delta: 0.0 };
    let identifier = identifierStart;
    for (const vector of dragVectors) {
      const result = await performTouchDragAttempt(vector, identifier);
      identifier += 1;
      if (result.delta > best.delta) {
        best = result;
      }
      if (best.delta >= 0.75) {
        break;
      }
    }
    return best;
  };

  let baselineTouchDrag = { delta: -1.0 };
  const controlTouchDragResults = [];
  if (touchApiReady) {
    baselineTouchDrag = await attemptTouchMovement();
    let touchIdentifier = 300;
    for (const button of controlButtons) {
      await dispatchTouchTap(button.x, button.y, touchIdentifier);
      touchIdentifier += 1;
      await page.waitForTimeout(140);
      const result = await attemptTouchMovement(touchIdentifier);
      touchIdentifier += 10;
      controlTouchDragResults.push({ button: button.name, delta: result.delta });
    }
  }

  await browser.close();

  if (hasCanvas < 1) {
    console.error('[ERROR] No canvas element detected.');
    process.exit(2);
  }
  if (!initialDebug.mazeAssetPresent || !initialDebug.scanAssetPresent || !initialDebug.collisionAssetPresent) {
    console.error('[ERROR] WASM VFS assets missing:', JSON.stringify(initialDebug));
    process.exit(5);
  }
  if (best.delta < 0.10) {
    console.error('[ERROR] Keyboard probe movement too small:', JSON.stringify(probeResults));
    process.exit(6);
  }
  if (intentTrueRatio < 0.75) {
    console.error(
      '[ERROR] Keyboard intent dropped while key held:',
      JSON.stringify({ key: best.key, intentSamples, intentTrueRatio }),
    );
    process.exit(7);
  }
  if (holdDelta < 0.20) {
    console.error(
      '[ERROR] Movement stalled during long key hold:',
      JSON.stringify({ key: best.key, holdDelta, probeResults }),
    );
    process.exit(8);
  }
  if (afterRelease.keyboardIntent) {
    console.error(
      '[ERROR] Keyboard state did not release after keyup:',
      JSON.stringify({ key: best.key, afterRelease }),
    );
    process.exit(9);
  }
  if (!postKeyDebug.audioEnabled || !postKeyDebug.scanSoundReady || !postKeyDebug.collisionSoundReady) {
    console.error(
      '[ERROR] Audio initialization/sound load failed after user input:',
      JSON.stringify(postKeyDebug),
    );
    process.exit(10);
  }
  if (baselineDrag.delta < 0.25) {
    console.error(
      '[ERROR] Baseline drag movement too small:',
      JSON.stringify(baselineDrag),
    );
    process.exit(11);
  }
  const blockedControl = controlDragResults.find((entry) => entry.delta < 0.25);
  if (blockedControl) {
    console.error(
      '[ERROR] Drag movement locked after control click:',
      JSON.stringify({ blockedControl, controlDragResults }),
    );
    process.exit(12);
  }
  if (touchApiReady) {
    if (baselineTouchDrag.delta < 0.25) {
      console.error(
        '[ERROR] Baseline touch-drag movement too small:',
        JSON.stringify(baselineTouchDrag),
      );
      process.exit(13);
    }
    const blockedTouchControl = controlTouchDragResults.find((entry) => entry.delta < 0.25);
    if (blockedTouchControl) {
      console.error(
        '[ERROR] Touch drag locked after control tap:',
        JSON.stringify({ blockedTouchControl, controlTouchDragResults }),
      );
      process.exit(14);
    }
  }
  if (errors.length > 0) {
    console.error('[ERROR] Browser/runtime errors detected:');
    for (const e of errors) console.error(` - ${e}`);
    process.exit(3);
  }
  console.log(
    '[OK] wasm e2e smoke passed (canvas, runtime, VFS assets, keyboard hold, mouse/touch drag-after-controls, and audio checks).',
  );
})().catch((err) => {
  console.error(err);
  process.exit(4);
});
NODE

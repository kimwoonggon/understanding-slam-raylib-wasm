/**
 * @file SlamApp.cpp
 * @brief Interactive app lifecycle, input, rendering, and audio integration.
 */

#include "app/SlamApp.h"

#include <algorithm>
#include <cmath>
#include <string>

#include <raylib.h>

#ifdef EMSCRIPTEN
#include <emscripten/emscripten.h>
#endif

#include "input/Motion.h"
#include "world/WorldLoader.h"
#include "app/AssetPaths.h"

namespace slam::app {
namespace {

/**
 * @brief Draw a labeled rectangular UI button.
 */
void DrawButton(const Rectangle& buttonRect, const char* labelText, Color bgColor, Color textColor) {
  DrawRectangleRec(buttonRect, bgColor);
  DrawRectangleLinesEx(buttonRect, 1.0F, textColor);
  DrawText(labelText, static_cast<int>(buttonRect.x) + 8, static_cast<int>(buttonRect.y) + 10, 16, textColor);
}

#ifdef EMSCRIPTEN
/**
 * @brief Ensure the WASM canvas remains keyboard-focusable.
 */
void EnsureWebCanvasFocusable() {
  EM_ASM({
    if (typeof Module !== 'undefined' && Module['canvas']) {
      const canvas = Module['canvas'];
      if (canvas.dataset.slamFocusHooked === '1') return;
      canvas.dataset.slamFocusHooked = '1';
      canvas.tabIndex = 1;
      canvas.style.outline = 'none';
      canvas.focus();
      const focus = () => canvas.focus();
      canvas.addEventListener('mousedown', focus);
      canvas.addEventListener('touchstart', focus, { passive: true });
      canvas.addEventListener('blur', () => {
        setTimeout(() => canvas.focus(), 0);
      });
    }
  });
}

/**
 * @brief Register browser gesture hooks to request audio unlock/initialization.
 */
void EnsureWebAudioUnlockHooks() {
  EM_ASM({
    if (typeof window === 'undefined') return;
    if (window.__slamAudioHooked === 1) return;
    window.__slamAudioHooked = 1;
    window.__slamAudioUnlockRequested = 0;

    const resumeKnownAudioContexts = () => {
      const contexts = [];
      if (typeof Module !== 'undefined') {
        if (Module.SDL2 && Module.SDL2.audioContext) contexts.push(Module.SDL2.audioContext);
        if (Module.audioContext) contexts.push(Module.audioContext);
      }
      if (window.AudioContext && window.__slamAudioContext instanceof window.AudioContext) {
        contexts.push(window.__slamAudioContext);
      }
      for (const ctx of contexts) {
        if (!ctx || typeof ctx.resume !== 'function') continue;
        if (ctx.state === 'suspended') {
          try { ctx.resume(); } catch (e) {}
        }
      }
    };

    const onUserGesture = () => {
      window.__slamAudioUnlockRequested = 1;
      resumeKnownAudioContexts();
    };

    if (typeof Module !== 'undefined' && Module.canvas) {
      Module.canvas.addEventListener('touchstart', onUserGesture, { passive: true });
      Module.canvas.addEventListener('mousedown', onUserGesture, { passive: true });
    }
    window.addEventListener('keydown', onUserGesture, { passive: true });
  });
}

/**
 * @brief Consume one pending browser gesture audio unlock request.
 */
bool ConsumeWebAudioUnlockRequest() {
  return EM_ASM_INT({
    if (typeof window === 'undefined') return 0;
    if (window.__slamAudioUnlockRequested === 1) {
      window.__slamAudioUnlockRequested = 0;
      return 1;
    }
    return 0;
  }) != 0;
}
#endif

}  // namespace

/**
 * @brief Construct and initialize runtime systems.
 * @param config Application configuration.
 */
SlamApp::SlamApp(const AppConfig& config)
    : config_(config),
      windowWidth_(config.world.width * config.screen.worldCellSize),
      windowHeight_(config.world.height * config.screen.worldCellSize),
      world_(core::WorldGrid::WithBorderWalls(config.world.width, config.world.height)),
      slamMap_(config.world.width, config.world.height),
      lidar_(config.lidar.maxRange, config.lidar.beamCount, config.lidar.stepSize),
      pose_({10.0, 10.0, 0.0}),
      showWorldMap_(config.world.showWorldByDefault) {
  InitWindow(windowWidth_, windowHeight_, "SLAM Understanding (Raylib C++)");
  SetTargetFPS(config_.screen.fps);
#ifdef EMSCRIPTEN
  EnsureWebCanvasFocusable();
  EnsureWebAudioUnlockHooks();
#endif
  controls_ = ui::CreateUiControlsForWindow(windowWidth_, windowHeight_);
  hitPixelOccupancy_.assign(static_cast<std::size_t>(windowWidth_ * windowHeight_), 0U);
  hitLayer_ = LoadRenderTexture(windowWidth_, windowHeight_);
  hitLayerReady_ = (hitLayer_.id != 0U);
  if (hitLayerReady_) {
    BeginTextureMode(hitLayer_);
    ClearBackground(BLANK);
    EndTextureMode();
  }
  InitializeWorld();
  const std::string scanPath = ResolveAssetPath("assets/sounds/scan_loop.wav");
  const std::string collisionPath = ResolveAssetPath("assets/sounds/collision_beep.wav");
  scanAssetPresent_ = FileExists(scanPath.c_str());
  collisionAssetPresent_ = FileExists(collisionPath.c_str());
#ifdef EMSCRIPTEN
  PublishWebDebugState(false, false);
#endif
#ifndef EMSCRIPTEN
  InitializeAudio();
#endif
}

/**
 * @brief Release graphics/audio resources.
 */
SlamApp::~SlamApp() {
  if (scanSoundReady_) {
    StopSound(scanSound_);
    UnloadSound(scanSound_);
  }
  if (collisionSoundReady_) {
    UnloadSound(collisionSound_);
  }
  if (IsAudioDeviceReady()) {
    CloseAudioDevice();
  }
  if (hitLayerReady_) {
    UnloadRenderTexture(hitLayer_);
    hitLayerReady_ = false;
  }
  if (IsWindowReady()) {
    CloseWindow();
  }
}

/**
 * @brief Run the interactive app loop.
 * @return Process-style exit code.
 */
int SlamApp::Run() {
  while (!WindowShouldClose()) {
    HandleInput();
    UpdateScan();
    UpdateAudio();
    DrawFrame();
  }
  return 0;
}

/**
 * @brief Load world geometry from configured image or fallback map.
 */
void SlamApp::InitializeWorld() {
  const std::string mazePath = ResolveAssetPath("assets/maze.png");
  mazeAssetPresent_ = FileExists(mazePath.c_str());
  if (mazeAssetPresent_) {
    world_ = world::BuildWorldFromImage(mazePath, config_.world.width, config_.world.height);
    return;
  }
  world_ = world::BuildDemoWorld(config_.world.width, config_.world.height);
}

/**
 * @brief Initialize audio device and load sound effects.
 */
void SlamApp::InitializeAudio() {
  if (!IsAudioDeviceReady()) {
    InitAudioDevice();
  }
  audioInitAttempted_ = true;
  if (!IsAudioDeviceReady()) {
    return;
  }

  const std::string scanPath = ResolveAssetPath("assets/sounds/scan_loop.wav");
  const std::string collisionPath = ResolveAssetPath("assets/sounds/collision_beep.wav");
  scanAssetPresent_ = FileExists(scanPath.c_str());
  collisionAssetPresent_ = FileExists(collisionPath.c_str());

  if (!scanSoundReady_ && scanAssetPresent_) {
    scanSound_ = LoadSound(scanPath.c_str());
    scanSoundReady_ = scanSound_.frameCount > 0;
  }
  if (!collisionSoundReady_ && collisionAssetPresent_) {
    collisionSound_ = LoadSound(collisionPath.c_str());
    collisionSoundReady_ = collisionSound_.frameCount > 0;
  }
  audioEnabled_ = scanSoundReady_ || collisionSoundReady_;
}

/**
 * @brief Reset reconstructed map state.
 */
void SlamApp::ResetMap() {
  slamMap_.Reset();
  ResetAccumulatedHitCache();
  wasAccumulating_ = false;
}

/**
 * @brief Process one frame of user input.
 */
void SlamApp::HandleInput() {
  movedThisFrame_ = false;
  collisionThisFrame_ = false;

  const bool iPressed = IsKeyPressed(KEY_I);
  const bool mPressed = IsKeyPressed(KEY_M);
  const bool gPressed = IsKeyPressed(KEY_G);
  const bool pPressed = IsKeyPressed(KEY_P);
  const bool wPressed = IsKeyPressed(KEY_W) || IsKeyPressed(KEY_UP);
  const bool sPressed = IsKeyPressed(KEY_S) || IsKeyPressed(KEY_DOWN);
  const bool aPressed = IsKeyPressed(KEY_A) || IsKeyPressed(KEY_LEFT);
  const bool dPressed = IsKeyPressed(KEY_D) || IsKeyPressed(KEY_RIGHT);
  const bool leftClicked = IsMouseButtonPressed(MOUSE_BUTTON_LEFT);
  const bool leftDown = IsMouseButtonDown(MOUSE_BUTTON_LEFT);
  const bool hasKeyboardIntent = IsKeyDown(KEY_W) || IsKeyDown(KEY_UP) || IsKeyDown(KEY_S) ||
                                 IsKeyDown(KEY_DOWN) || IsKeyDown(KEY_A) || IsKeyDown(KEY_LEFT) ||
                                 IsKeyDown(KEY_D) || IsKeyDown(KEY_RIGHT);
  const bool keyboardPressEvent = wPressed || sPressed || aPressed || dPressed;
  const bool userInteractionEvent = leftClicked || leftDown || iPressed || mPressed || gPressed || keyboardPressEvent;
  const Vector2 mousePos = GetMousePosition();

  bool webAudioUnlockRequested = false;
#ifdef EMSCRIPTEN
  webAudioUnlockRequested = ConsumeWebAudioUnlockRequest();
#endif

  if (!audioEnabled_ && (userInteractionEvent || webAudioUnlockRequested)) {
    InitializeAudio();
  }

#ifdef EMSCRIPTEN
  if (leftClicked) {
    EM_ASM({
      if (typeof Module !== 'undefined' && Module['canvas']) Module['canvas'].focus();
    });
  }
#endif

  if (ui::ShouldResetFromInputs(iPressed, leftClicked, mousePos, controls_.reset)) {
    ResetMap();
  }

  const bool toggleWorldClicked = leftClicked && CheckCollisionPointRec(mousePos, controls_.toggleWorld);
  const bool toggleAccClicked = leftClicked && CheckCollisionPointRec(mousePos, controls_.accumulate);
  if (mPressed || toggleWorldClicked) {
    showWorldMap_ = !showWorldMap_;
  }
  if (gPressed || toggleAccClicked) {
    accumulateHits_ = !accumulateHits_;
  }

  if (pPressed) {
    cursorLocked_ = !cursorLocked_;
    if (cursorLocked_) {
      DisableCursor();
#ifdef EMSCRIPTEN
      EM_ASM({
        if (typeof Module !== 'undefined' && Module['canvas'] &&
            Module['canvas'].requestPointerLock) {
          Module['canvas'].requestPointerLock();
        }
      });
#endif
    } else {
      EnableCursor();
#ifdef EMSCRIPTEN
      EM_ASM({
        if (typeof document !== 'undefined' && document.exitPointerLock) {
          document.exitPointerLock();
        }
      });
#endif
    }
  }

  const bool draggingNow = leftDown && !IsMouseOnControl(mousePos);

  if (hasKeyboardIntent) {
    HandleKeyboardMotion();
  } else if (draggingNow) {
    HandleMouseDrag(mousePos);
  }

#ifdef EMSCRIPTEN
  PublishWebDebugState(hasKeyboardIntent, draggingNow);
#endif
}

/**
 * @brief Return whether mouse position overlaps a control button.
 */
bool SlamApp::IsMouseOnControl(Vector2 mousePos) const {
  return CheckCollisionPointRec(mousePos, controls_.reset) ||
         CheckCollisionPointRec(mousePos, controls_.toggleWorld) ||
         CheckCollisionPointRec(mousePos, controls_.accumulate);
}

/**
 * @brief Apply keyboard-driven motion with collision handling.
 */
void SlamApp::HandleKeyboardMotion() {
  const core::RobotPose motionCandidate = input::HandleMotion(
      pose_,
      config_.motion.keyboardSpeed,
      IsKeyDown(KEY_W) || IsKeyDown(KEY_UP),
      IsKeyDown(KEY_S) || IsKeyDown(KEY_DOWN),
      IsKeyDown(KEY_A) || IsKeyDown(KEY_LEFT),
      IsKeyDown(KEY_D) || IsKeyDown(KEY_RIGHT));
  if (motionCandidate.x != pose_.x || motionCandidate.y != pose_.y) {
    const core::RobotPose previous = pose_;
    if (!world_.IsObstacle(static_cast<int>(motionCandidate.x), static_cast<int>(motionCandidate.y))) {
      pose_ = motionCandidate;
      movedThisFrame_ = movedThisFrame_ || (pose_.x != previous.x || pose_.y != previous.y);
    } else {
      collisionThisFrame_ = true;
    }
  }
}

/**
 * @brief Apply drag-driven motion with collision handling.
 */
void SlamApp::HandleMouseDrag(Vector2 mousePos) {
  const core::RobotPose oldPose = pose_;
  const int targetX = static_cast<int>(mousePos.x) / config_.screen.worldCellSize;
  const int targetY = static_cast<int>(mousePos.y) / config_.screen.worldCellSize;
  pose_ = input::ApplyMouseDragToPose(pose_, targetX, targetY, world_);
  movedThisFrame_ = movedThisFrame_ || (pose_.x != oldPose.x || pose_.y != oldPose.y);
  if (pose_.x == oldPose.x && pose_.y == oldPose.y &&
      (targetX != static_cast<int>(oldPose.x) || targetY != static_cast<int>(oldPose.y))) {
    collisionThisFrame_ = true;
  }
}

#ifdef EMSCRIPTEN
/**
 * @brief Publish runtime debug state for browser automation and diagnostics.
 */
void SlamApp::PublishWebDebugState(bool hasKeyboardIntent, bool draggingNow) const {
  const int poseXMilli = static_cast<int>(std::lround(pose_.x * 1000.0));
  const int poseYMilli = static_cast<int>(std::lround(pose_.y * 1000.0));
  const int fps = GetFPS();
  const int hitHistorySize = static_cast<int>(hitHistory_.size());
  EM_ASM({
    if (typeof window === 'undefined') return;
    if (!window.__slamDebug) window.__slamDebug = {};
    window.__slamDebug.poseX = $0 / 1000.0;
    window.__slamDebug.poseY = $1 / 1000.0;
    window.__slamDebug.keyboardIntent = !!$2;
    window.__slamDebug.dragging = !!$3;
    window.__slamDebug.audioEnabled = !!$4;
    window.__slamDebug.mazeAssetPresent = !!$5;
    window.__slamDebug.scanAssetPresent = !!$6;
    window.__slamDebug.collisionAssetPresent = !!$7;
    window.__slamDebug.scanSoundReady = !!$8;
    window.__slamDebug.collisionSoundReady = !!$9;
    window.__slamDebug.audioInitAttempted = !!$10;
    window.__slamDebug.audioDeviceReady = !!$11;
    window.__slamDebug.fps = $12;
    window.__slamDebug.hitHistorySize = $13;
    window.__slamDebug.accumulateHits = !!$14;
  },
         poseXMilli,
         poseYMilli,
         hasKeyboardIntent ? 1 : 0,
         draggingNow ? 1 : 0,
         audioEnabled_ ? 1 : 0,
         mazeAssetPresent_ ? 1 : 0,
         scanAssetPresent_ ? 1 : 0,
         collisionAssetPresent_ ? 1 : 0,
         scanSoundReady_ ? 1 : 0,
         collisionSoundReady_ ? 1 : 0,
         audioInitAttempted_ ? 1 : 0,
         IsAudioDeviceReady() ? 1 : 0,
         fps,
         hitHistorySize,
         accumulateHits_ ? 1 : 0);
}
#endif

/**
 * @brief Perform one lidar scan and map integration update.
 */
void SlamApp::UpdateScan() {
  latestScan_ = lidar_.Scan(world_, pose_);
  slamMap_.IntegrateScan(pose_, latestScan_);
  latestRays_ = render::ScanSamplesToPixels(pose_, latestScan_, config_.screen.worldCellSize, 0);

  std::vector<Vector2> currentHits;
  currentHits.reserve(latestRays_.size());
  for (const render::PixelRay& ray : latestRays_) {
    if (ray.hit) {
      currentHits.push_back(ray.end);
    }
  }

  if (!accumulateHits_) {
    if (wasAccumulating_) {
      ResetAccumulatedHitCache();
      wasAccumulating_ = false;
    }
    hitHistory_ = std::move(currentHits);
    return;
  }

  pendingAccumulatedDrawHits_.clear();
  if (!wasAccumulating_) {
    const std::vector<Vector2> seedHits = hitHistory_;
    ResetAccumulatedHitCache();
    for (const Vector2& point : seedHits) {
      if (render::TryMarkHitPixel(hitPixelOccupancy_, windowWidth_, windowHeight_, point)) {
        hitHistory_.push_back(point);
        pendingAccumulatedDrawHits_.push_back(point);
      }
    }
    wasAccumulating_ = true;
  }

  for (const Vector2& point : currentHits) {
    if (render::TryMarkHitPixel(hitPixelOccupancy_, windowWidth_, windowHeight_, point)) {
      hitHistory_.push_back(point);
      pendingAccumulatedDrawHits_.push_back(point);
    }
  }
  FlushAccumulatedHitDraws();
}

void SlamApp::ResetAccumulatedHitCache() {
  hitHistory_.clear();
  pendingAccumulatedDrawHits_.clear();
  std::fill(hitPixelOccupancy_.begin(), hitPixelOccupancy_.end(), 0U);
  if (hitLayerReady_) {
    BeginTextureMode(hitLayer_);
    ClearBackground(BLANK);
    EndTextureMode();
  }
}

void SlamApp::FlushAccumulatedHitDraws() {
  if (!hitLayerReady_ || pendingAccumulatedDrawHits_.empty()) {
    return;
  }
  BeginTextureMode(hitLayer_);
  for (const Vector2& point : pendingAccumulatedDrawHits_) {
    DrawCircleV(point, 2.0F, render::Palette::kHit);
  }
  EndTextureMode();
  pendingAccumulatedDrawHits_.clear();
}

/**
 * @brief Render world/map, rays, hits, robot, and controls.
 */
void SlamApp::DrawFrame() const {
  BeginDrawing();
  ClearBackground(render::Palette::kBackground);

  if (showWorldMap_) {
    render::DrawWorld(world_, config_.screen.worldCellSize, 0);
  } else {
    render::DrawMap(slamMap_, config_.screen.worldCellSize, 0);
  }

  for (const render::PixelRay& ray : latestRays_) {
    DrawLineV(ray.start, ray.end, render::Palette::kLaser);
  }
  if (accumulateHits_ && hitLayerReady_) {
    DrawTextureRec(
        hitLayer_.texture,
        Rectangle{0.0F, 0.0F, static_cast<float>(hitLayer_.texture.width), -static_cast<float>(hitLayer_.texture.height)},
        Vector2{0.0F, 0.0F},
        WHITE);
  } else {
    for (const Vector2& hit : hitHistory_) {
      DrawCircleV(hit, 2.0F, render::Palette::kHit);
    }
  }

  DrawRectangle(
      static_cast<int>(pose_.x * static_cast<float>(config_.screen.worldCellSize)) - 3,
      static_cast<int>(pose_.y * static_cast<float>(config_.screen.worldCellSize)) - 3,
      6,
      6,
      render::Palette::kRobot);

  const std::string worldText = std::string("WORLD ") + (showWorldMap_ ? "ON" : "OFF") + " (M)";
  const std::string hitText = std::string("GREEN ") + (accumulateHits_ ? "ACC" : "LIVE") + " (G)";
  DrawButton(controls_.reset, "RESET (I)", Color{40, 40, 40, 255}, render::Palette::kText);
  DrawButton(controls_.toggleWorld, worldText.c_str(), Color{40, 40, 40, 255}, render::Palette::kText);
  DrawButton(controls_.accumulate, hitText.c_str(), Color{40, 40, 40, 255}, render::Palette::kText);
  DrawText(TextFormat("FPS: %i", GetFPS()), 10, 10, 20, GREEN);

  EndDrawing();
}

/**
 * @brief Update scan/collision audio playback for current frame state.
 */
void SlamApp::UpdateAudio() {
  if (!audioEnabled_) {
    return;
  }

  if (scanSoundReady_) {
    if (movedThisFrame_ && !scanPlaying_) {
      PlaySound(scanSound_);
      scanPlaying_ = true;
    } else if (!movedThisFrame_ && scanPlaying_) {
      StopSound(scanSound_);
      scanPlaying_ = false;
    }
  }

  if (collisionThisFrame_ && collisionSoundReady_) {
    const double now = GetTime();
    if (now - lastCollisionTime_ >= collisionCooldownSec_) {
      PlaySound(collisionSound_);
      lastCollisionTime_ = now;
    }
  }
}

}  // namespace slam::app

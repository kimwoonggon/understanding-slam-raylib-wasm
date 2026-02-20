/**
 * @file SlamApp.cpp
 * @brief Interactive app lifecycle, input, rendering, and audio integration.
 */

#include "app/SlamApp.h"

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

void DrawButton(const Rectangle& buttonRect, const char* labelText, Color bgColor, Color textColor) {
  DrawRectangleRec(buttonRect, bgColor);
  DrawRectangleLinesEx(buttonRect, 1.0F, textColor);
  DrawText(labelText, static_cast<int>(buttonRect.x) + 8, static_cast<int>(buttonRect.y) + 10, 16, textColor);
}

#ifdef EMSCRIPTEN
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
#endif

}  // namespace

SlamApp::SlamApp(const AppConfig& config)
    : config_(config),
      world_(core::WorldGrid::WithBorderWalls(config.world.width, config.world.height)),
      slamMap_(config.world.width, config.world.height),
      lidar_(config.lidar.maxRange, config.lidar.beamCount, config.lidar.stepSize),
      pose_({10.0, 10.0, 0.0}),
      showWorldMap_(config.world.showWorldByDefault) {
  const int windowWidth = config_.world.width * config_.screen.worldCellSize;
  const int windowHeight = config_.world.height * config_.screen.worldCellSize;
  InitWindow(windowWidth, windowHeight, "SLAM Understanding (Raylib C++)");
  SetTargetFPS(config_.screen.fps);
#ifdef EMSCRIPTEN
  EnsureWebCanvasFocusable();
#endif
  controls_ = ui::CreateUiControlsForWindow(windowWidth, windowHeight);
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
  if (IsWindowReady()) {
    CloseWindow();
  }
}

int SlamApp::Run() {
  while (!WindowShouldClose()) {
    HandleInput();
    UpdateScan();
    UpdateAudio();
    DrawFrame();
  }
  return 0;
}

void SlamApp::InitializeWorld() {
  const std::string mazePath = ResolveAssetPath("assets/maze.png");
  mazeAssetPresent_ = FileExists(mazePath.c_str());
  if (mazeAssetPresent_) {
    world_ = world::BuildWorldFromImage(mazePath, config_.world.width, config_.world.height);
    return;
  }
  world_ = world::BuildDemoWorld(config_.world.width, config_.world.height);
}

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

void SlamApp::ResetMap() {
  slamMap_.Reset();
  hitHistory_.clear();
}

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
  const Vector2 mousePos = GetMousePosition();

  if (!audioEnabled_ && (leftClicked || iPressed || mPressed || gPressed || keyboardPressEvent)) {
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

bool SlamApp::IsMouseOnControl(Vector2 mousePos) const {
  return CheckCollisionPointRec(mousePos, controls_.reset) ||
         CheckCollisionPointRec(mousePos, controls_.toggleWorld) ||
         CheckCollisionPointRec(mousePos, controls_.accumulate);
}

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
void SlamApp::PublishWebDebugState(bool hasKeyboardIntent, bool draggingNow) const {
  const int poseXMilli = static_cast<int>(std::lround(pose_.x * 1000.0));
  const int poseYMilli = static_cast<int>(std::lround(pose_.y * 1000.0));
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
         collisionSoundReady_ ? 1 : 0);
}
#endif

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
  hitHistory_ = render::UpdateHitPointHistory(hitHistory_, currentHits, accumulateHits_);
}

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
  for (const Vector2& hit : hitHistory_) {
    DrawCircleV(hit, 2.0F, render::Palette::kHit);
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

  EndDrawing();
}

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

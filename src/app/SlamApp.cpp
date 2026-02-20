#include "app/SlamApp.h"

#include <cmath>
#include <string>

#include <raylib.h>

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

}  // namespace

SlamApp::SlamApp(const AppConfig& config)
    : config_(config),
      world_(core::WorldGrid::WithBorderWalls(config.world.width, config.world.height)),
      slamMap_(config.world.width, config.world.height),
      lidar_(config.lidar.maxRange, config.lidar.beamCount, config.lidar.stepSize),
      pose_({10.0F, 10.0F, 0.0F}),
      showWorldMap_(config.world.showWorldByDefault) {
  const int windowWidth = config_.world.width * config_.screen.worldCellSize;
  const int windowHeight = config_.world.height * config_.screen.worldCellSize;
  InitWindow(windowWidth, windowHeight, "SLAM Understanding (Raylib C++)");
  SetTargetFPS(config_.screen.fps);
  controls_ = ui::CreateUiControlsForWindow(windowWidth, windowHeight);
  InitializeWorld();
  InitializeAudio();
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
  if (FileExists(mazePath.c_str())) {
    world_ = world::BuildWorldFromImage(mazePath, config_.world.width, config_.world.height);
    return;
  }
  world_ = world::BuildDemoWorld(config_.world.width, config_.world.height);
}

void SlamApp::InitializeAudio() {
  InitAudioDevice();
  if (!IsAudioDeviceReady()) {
    return;
  }

  const std::string scanPath = ResolveAssetPath("assets/sounds/scan_loop.wav");
  const std::string collisionPath = ResolveAssetPath("assets/sounds/collision_beep.wav");

  if (FileExists(scanPath.c_str())) {
    scanSound_ = LoadSound(scanPath.c_str());
    scanSoundReady_ = scanSound_.frameCount > 0;
  }
  if (FileExists(collisionPath.c_str())) {
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
  const bool leftClicked = IsMouseButtonPressed(MOUSE_BUTTON_LEFT);
  const Vector2 mousePos = GetMousePosition();

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
      movedThisFrame_ = pose_.x != previous.x || pose_.y != previous.y;
    } else {
      collisionThisFrame_ = true;
    }
  }

  const bool mouseOnControl = CheckCollisionPointRec(mousePos, controls_.reset) ||
                              CheckCollisionPointRec(mousePos, controls_.toggleWorld) ||
                              CheckCollisionPointRec(mousePos, controls_.accumulate);
  if (IsMouseButtonDown(MOUSE_BUTTON_LEFT) && !mouseOnControl) {
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
}

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

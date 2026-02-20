#pragma once

#include <vector>

#include <raylib.h>

#include "app/Config.h"
#include "core/OccupancyGridMap.h"
#include "core/SimulatedLidar.h"
#include "core/Types.h"
#include "core/WorldGrid.h"
#include "render/Renderer.h"
#include "ui/UiControls.h"

/**
 * @file SlamApp.h
 * @brief Interactive raylib SLAM application orchestration.
 */

namespace slam::app {

/**
 * @brief Owns application lifecycle, input handling, scan updates, and rendering.
 */
class SlamApp {
 public:
  /**
   * @brief Construct the app with runtime configuration.
   */
  explicit SlamApp(const AppConfig& config);
  /**
   * @brief Release runtime resources.
   */
  ~SlamApp();

  /**
   * @brief Execute the interactive frame loop.
   * @return Process-style exit code.
   */
  int Run();

 private:
  void InitializeWorld();
  void InitializeAudio();
  void ResetMap();
  void HandleInput();
  bool IsMouseOnControl(Vector2 mousePos) const;
  void HandleKeyboardMotion();
  void HandleMouseDrag(Vector2 mousePos);
#ifdef EMSCRIPTEN
  void PublishWebDebugState(bool hasKeyboardIntent, bool draggingNow) const;
#endif
  void UpdateScan();
  void UpdateAudio();
  void DrawFrame() const;

  AppConfig config_;
  core::WorldGrid world_;
  core::OccupancyGridMap slamMap_;
  core::SimulatedLidar lidar_;
  core::RobotPose pose_{};
  ui::UiControls controls_{};

  bool showWorldMap_ = false;
  bool accumulateHits_ = false;
  bool cursorLocked_ = false;
  bool movedThisFrame_ = false;
  bool collisionThisFrame_ = false;
  std::vector<Vector2> hitHistory_;
  std::vector<core::ScanSample> latestScan_;
  std::vector<render::PixelRay> latestRays_;

  bool audioEnabled_ = false;
  bool audioInitAttempted_ = false;
  bool mazeAssetPresent_ = false;
  bool scanAssetPresent_ = false;
  bool collisionAssetPresent_ = false;
  bool scanSoundReady_ = false;
  bool collisionSoundReady_ = false;
  bool scanPlaying_ = false;
  double lastCollisionTime_ = -10000.0;
  double collisionCooldownSec_ = 0.2;
  Sound scanSound_{};
  Sound collisionSound_{};
};

}  // namespace slam::app

#pragma once

/**
 * @file Config.h
 * @brief Runtime configuration structures for the SLAM app.
 */

namespace slam::app {

/**
 * @brief Screen and frame pacing parameters.
 */
struct ScreenConfig {
  /// Pixel size per world cell.
  int worldCellSize = 8;
  /// Target frames per second.
  int fps = 60;
};

/**
 * @brief World dimensions and default visibility behavior.
 */
struct WorldConfig {
  /// World width in grid cells.
  int width = 120;
  /// World height in grid cells.
  int height = 80;
  /// True to show world map at startup.
  bool showWorldByDefault = false;
};

/**
 * @brief Lidar simulation parameters.
 */
struct LidarConfig {
  /// Maximum beam range in grid units.
  double maxRange = 30.0;
  /// Number of beams per scan.
  int beamCount = 72;
  /// Ray-march step size in grid units.
  double stepSize = 1.0;
};

/**
 * @brief Robot motion parameters.
 */
struct MotionConfig {
  /// Keyboard translation speed in grid units per frame.
  double keyboardSpeed = 0.5;
};

/**
 * @brief Aggregate application configuration.
 */
struct AppConfig {
  ScreenConfig screen{};
  WorldConfig world{};
  LidarConfig lidar{};
  MotionConfig motion{};

  /**
   * @brief Return the default app configuration.
   */
  static AppConfig Default() {
    return AppConfig{};
  }
};

}  // namespace slam::app

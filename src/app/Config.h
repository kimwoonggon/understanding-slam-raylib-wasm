#pragma once

namespace slam::app {

struct ScreenConfig {
  int worldCellSize = 8;
  int fps = 60;
};

struct WorldConfig {
  int width = 120;
  int height = 80;
  bool showWorldByDefault = false;
};

struct LidarConfig {
  double maxRange = 30.0;
  int beamCount = 72;
  double stepSize = 1.0;
};

struct MotionConfig {
  double keyboardSpeed = 0.5;
};

struct AppConfig {
  ScreenConfig screen{};
  WorldConfig world{};
  LidarConfig lidar{};
  MotionConfig motion{};

  static AppConfig Default() {
    return AppConfig{};
  }
};

}  // namespace slam::app

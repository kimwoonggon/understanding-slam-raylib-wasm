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
  float maxRange = 30.0F;
  int beamCount = 72;
  float stepSize = 1.0F;
};

struct MotionConfig {
  float keyboardSpeed = 0.5F;
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

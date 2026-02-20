#pragma once

#include <cstdint>
#include <vector>

namespace slam::core {

class WorldGrid {
 public:
  WorldGrid(int width, int height);

  static WorldGrid WithBorderWalls(int width, int height);

  bool InBounds(int x, int y) const;
  void SetObstacle(int x, int y);
  void AddRectangle(int x, int y, int width, int height);
  bool IsObstacle(int x, int y) const;

  int Width() const { return width_; }
  int Height() const { return height_; }
  const std::vector<std::uint8_t>& ObstacleData() const { return obstacles_; }

 private:
  int Index(int x, int y) const;

  int width_ = 0;
  int height_ = 0;
  std::vector<std::uint8_t> obstacles_;
};

}  // namespace slam::core

#pragma once

#include <cstdint>
#include <vector>

#include "core/Types.h"

namespace slam::core {

class OccupancyGridMap {
 public:
  OccupancyGridMap(int width, int height);

  void Reset();
  std::int16_t ValueAt(int x, int y) const;
  void IntegrateScan(const RobotPose& pose, const std::vector<ScanSample>& scan);

  int Width() const { return width_; }
  int Height() const { return height_; }
  const std::vector<std::int16_t>& Data() const { return grid_; }

 private:
  bool InBounds(int x, int y) const;
  int Index(int x, int y) const;

  int width_ = 0;
  int height_ = 0;
  std::vector<std::int16_t> grid_;
};

}  // namespace slam::core

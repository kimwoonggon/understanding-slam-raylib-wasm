#include "core/OccupancyGridMap.h"

#include <cmath>
#include <stdexcept>

namespace slam::core {
namespace {

std::vector<std::pair<int, int>> Bresenham(std::pair<int, int> start, std::pair<int, int> end) {
  int x0 = start.first;
  int y0 = start.second;
  const int x1 = end.first;
  const int y1 = end.second;

  std::vector<std::pair<int, int>> points;
  const int dx = std::abs(x1 - x0);
  const int dy = std::abs(y1 - y0);
  const int xStep = (x0 < x1) ? 1 : -1;
  const int yStep = (y0 < y1) ? 1 : -1;

  int err = dx - dy;
  while (true) {
    points.push_back({x0, y0});
    if (x0 == x1 && y0 == y1) {
      break;
    }
    const int errTwice = 2 * err;
    if (errTwice > -dy) {
      err -= dy;
      x0 += xStep;
    }
    if (errTwice < dx) {
      err += dx;
      y0 += yStep;
    }
  }
  return points;
}

}  // namespace

OccupancyGridMap::OccupancyGridMap(int width, int height)
    : width_(width),
      height_(height),
      grid_(static_cast<std::size_t>(width * height), kUnknown) {
  if (width <= 0 || height <= 0) {
    throw std::invalid_argument("OccupancyGridMap dimensions must be positive");
  }
}

void OccupancyGridMap::Reset() {
  std::fill(grid_.begin(), grid_.end(), kUnknown);
}

std::int16_t OccupancyGridMap::ValueAt(int x, int y) const {
  return grid_[static_cast<std::size_t>(Index(x, y))];
}

void OccupancyGridMap::IntegrateScan(const RobotPose& pose, const std::vector<ScanSample>& scan) {
  const std::pair<int, int> start{static_cast<int>(pose.x), static_cast<int>(pose.y)};

  for (const ScanSample& sample : scan) {
    const float angle = pose.theta + sample.relativeAngle;
    const int endX = static_cast<int>(pose.x + std::cos(angle) * sample.distance);
    const int endY = static_cast<int>(pose.y + std::sin(angle) * sample.distance);
    const std::vector<std::pair<int, int>> ray = Bresenham(start, {endX, endY});

    const std::size_t freeLimit = sample.hit ? (ray.size() > 1 ? ray.size() - 1 : 0) : ray.size();
    for (std::size_t i = 1; i < freeLimit; ++i) {
      const auto [x, y] = ray[i];
      if (InBounds(x, y)) {
        grid_[static_cast<std::size_t>(Index(x, y))] = kFree;
      }
    }

    if (sample.hit && InBounds(endX, endY)) {
      grid_[static_cast<std::size_t>(Index(endX, endY))] = kOccupied;
    }
  }
}

bool OccupancyGridMap::InBounds(int x, int y) const {
  return x >= 0 && x < width_ && y >= 0 && y < height_;
}

int OccupancyGridMap::Index(int x, int y) const {
  return y * width_ + x;
}

}  // namespace slam::core

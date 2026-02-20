#pragma once

#include <cstdint>
#include <vector>

#include "core/Types.h"

/**
 * @file OccupancyGridMap.h
 * @brief Occupancy-grid map integration primitives.
 */

namespace slam::core {

/**
 * @brief Reconstructed occupancy map updated by lidar scans.
 */
class OccupancyGridMap {
 public:
  /**
   * @brief Construct an occupancy map initialized to unknown.
   * @param width Map width in cells.
   * @param height Map height in cells.
   */
  OccupancyGridMap(int width, int height);

  /// Reset all cells back to unknown.
  void Reset();
  /// Read one map cell value.
  std::int16_t ValueAt(int x, int y) const;
  /**
   * @brief Integrate one lidar scan into the map.
   * @param pose Robot pose at scan time.
   * @param scan Beam samples.
   */
  void IntegrateScan(const RobotPose& pose, const std::vector<ScanSample>& scan);

  /// @return Map width in cells.
  int Width() const { return width_; }
  /// @return Map height in cells.
  int Height() const { return height_; }
  /// @return Raw occupancy buffer in row-major order.
  const std::vector<std::int16_t>& Data() const { return grid_; }

 private:
  /**
   * @brief Check whether a coordinate is inside map bounds.
   */
  bool InBounds(int x, int y) const;
  /**
   * @brief Convert 2D coordinate to row-major index.
   */
  int Index(int x, int y) const;

  int width_ = 0;
  int height_ = 0;
  std::vector<std::int16_t> grid_;
};

}  // namespace slam::core

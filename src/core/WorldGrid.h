#pragma once

#include <cstdint>
#include <vector>

/**
 * @file WorldGrid.h
 * @brief Ground-truth obstacle grid utilities.
 */

namespace slam::core {

/**
 * @brief Obstacle grid used as the simulated environment.
 */
class WorldGrid {
 public:
  /**
   * @brief Construct an empty world grid.
   * @param width Number of grid columns.
   * @param height Number of grid rows.
   */
  WorldGrid(int width, int height);

  /**
   * @brief Create a world with border walls enabled.
   * @param width Number of grid columns.
   * @param height Number of grid rows.
   * @return Grid initialized with border obstacles.
   */
  static WorldGrid WithBorderWalls(int width, int height);

  /**
   * @brief Check whether a cell coordinate lies in bounds.
   */
  bool InBounds(int x, int y) const;
  /**
   * @brief Mark one cell as an obstacle.
   */
  void SetObstacle(int x, int y);
  /**
   * @brief Mark a rectangular region as obstacles.
   */
  void AddRectangle(int x, int y, int width, int height);
  /**
   * @brief Return true if the cell is blocked or outside the grid.
   */
  bool IsObstacle(int x, int y) const;

  /// @return Grid width in cells.
  int Width() const { return width_; }
  /// @return Grid height in cells.
  int Height() const { return height_; }
  /// @return Raw obstacle buffer in row-major order.
  const std::vector<std::uint8_t>& ObstacleData() const { return obstacles_; }

 private:
  /**
   * @brief Convert 2D coordinate to row-major index.
   */
  int Index(int x, int y) const;

  int width_ = 0;
  int height_ = 0;
  std::vector<std::uint8_t> obstacles_;
};

}  // namespace slam::core

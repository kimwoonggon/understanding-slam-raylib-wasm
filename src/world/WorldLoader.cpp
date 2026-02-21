/**
 * @file WorldLoader.cpp
 * @brief World construction from procedural defaults or map images.
 */

#include "world/WorldLoader.h"

#include <raylib.h>

namespace slam::world {

/**
 * @brief Build the procedural fallback world layout.
 */
core::WorldGrid BuildDemoWorld(int width, int height) {
  core::WorldGrid world = core::WorldGrid::WithBorderWalls(width, height);
  world.AddRectangle(20, 12, 15, 3);
  world.AddRectangle(60, 18, 10, 18);
  world.AddRectangle(35, 45, 30, 4);
  world.AddRectangle(80, 55, 18, 10);
  return world;
}

/**
 * @brief Build world grid from a map image.
 * @param imagePath Source image path.
 * @param width Target world width in cells.
 * @param height Target world height in cells.
 * @return World grid generated from rasterized obstacles.
 */
core::WorldGrid BuildWorldFromImage(const std::string& imagePath, int width, int height) {
  Image image = LoadImage(imagePath.c_str());
  if (image.data == nullptr) {
    return BuildDemoWorld(width, height);
  }

  if (image.width != width || image.height != height) {
    // Match pygame.transform.scale behavior for map rasterization parity.
    ImageResizeNN(&image, width, height);
  }

  core::WorldGrid world(width, height);
  Color* pixels = LoadImageColors(image);
  if (pixels != nullptr) {
    for (int y = 0; y < height; ++y) {
      for (int x = 0; x < width; ++x) {
        const Color color = pixels[y * width + x];
        if (color.r < 32 && color.g < 32 && color.b < 32) {
          world.SetObstacle(x, y);
        }
      }
    }
    UnloadImageColors(pixels);
  }
  UnloadImage(image);
  return world;
}

}  // namespace slam::world

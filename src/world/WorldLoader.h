#pragma once

#include <string>

#include "core/WorldGrid.h"

/**
 * @file WorldLoader.h
 * @brief World creation utilities from procedural or image sources.
 */

namespace slam::world {

/**
 * @brief Build the fallback demo world layout.
 */
core::WorldGrid BuildDemoWorld(int width, int height);
/**
 * @brief Build a world by thresholding a map image.
 */
core::WorldGrid BuildWorldFromImage(const std::string& imagePath, int width, int height);

}  // namespace slam::world

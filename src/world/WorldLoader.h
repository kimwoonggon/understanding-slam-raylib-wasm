#pragma once

#include <string>

#include "core/WorldGrid.h"

namespace slam::world {

core::WorldGrid BuildDemoWorld(int width, int height);
core::WorldGrid BuildWorldFromImage(const std::string& imagePath, int width, int height);

}  // namespace slam::world

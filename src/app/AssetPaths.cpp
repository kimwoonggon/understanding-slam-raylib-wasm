/**
 * @file AssetPaths.cpp
 * @brief Asset path resolution implementation across native and WASM layouts.
 */

#include "app/AssetPaths.h"

#include <filesystem>
#include <vector>

namespace slam::app {

/**
 * @brief Resolve an asset path across known runtime locations.
 * @param relativePath Project-relative asset path.
 * @return First existing candidate path, else the original input.
 */
std::string ResolveAssetPath(const std::string& relativePath) {
  namespace fs = std::filesystem;

  std::vector<std::string> candidates;
  candidates.reserve(4);
#ifdef EMSCRIPTEN
  if (!relativePath.empty() && relativePath.front() != '/') {
    candidates.push_back(std::string("/") + relativePath);
  }
#endif
  candidates.push_back(relativePath);
  candidates.push_back(std::string("../") + relativePath);
  candidates.push_back(std::string(SLAM_PROJECT_ROOT) + "/" + relativePath);

  for (const auto& candidate : candidates) {
    std::error_code ec;
    if (fs::exists(fs::path(candidate), ec) && !ec) {
      return candidate;
    }
  }
  return relativePath;
}

}  // namespace slam::app

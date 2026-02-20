#include "app/AssetPaths.h"

#include <filesystem>
#include <vector>

namespace slam::app {

std::string ResolveAssetPath(const std::string& relativePath) {
  namespace fs = std::filesystem;

  const std::vector<std::string> candidates = {
      relativePath,
      std::string("../") + relativePath,
      std::string(SLAM_PROJECT_ROOT) + "/" + relativePath,
  };

  for (const auto& candidate : candidates) {
    std::error_code ec;
    if (fs::exists(fs::path(candidate), ec) && !ec) {
      return candidate;
    }
  }
  return relativePath;
}

}  // namespace slam::app

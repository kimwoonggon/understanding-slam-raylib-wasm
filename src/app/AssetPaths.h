#pragma once

#include <string>

/**
 * @file AssetPaths.h
 * @brief Runtime asset path resolution helpers.
 */

namespace slam::app {

/**
 * @brief Resolve an asset path across known runtime locations.
 * @param relativePath Project-relative asset path.
 * @return First existing path candidate, else the input path.
 */
std::string ResolveAssetPath(const std::string& relativePath);

}  // namespace slam::app

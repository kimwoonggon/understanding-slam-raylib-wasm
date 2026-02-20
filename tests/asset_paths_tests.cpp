#include <cstdlib>
#include <filesystem>
#include <fstream>
#include <functional>
#include <iostream>
#include <stdexcept>
#include <string>
#include <vector>

#include "app/AssetPaths.h"

namespace {

struct TestResult {
  std::string name;
  bool passed = false;
  std::string message;
};

#define ASSERT_TRUE(cond, msg) \
  do {                         \
    if (!(cond)) {             \
      throw std::runtime_error(msg); \
    }                          \
  } while (false)

TestResult Run(const std::string& name, const std::function<void()>& fn) {
  try {
    fn();
    return {name, true, ""};
  } catch (const std::exception& ex) {
    return {name, false, ex.what()};
  }
}

void TestResolveAssetPathPrefersExistingRelativePath() {
  const std::filesystem::path tmpDir = std::filesystem::path("tmp_test_assets");
  const std::filesystem::path mazePath = tmpDir / "maze.png";
  std::filesystem::create_directories(tmpDir);
  {
    std::ofstream out(mazePath.string());
    out << "x";
  }

  const std::string resolved = slam::app::ResolveAssetPath("tmp_test_assets/maze.png");
  ASSERT_TRUE(
      std::filesystem::equivalent(std::filesystem::path(resolved), mazePath),
      "resolver must return existing relative file");

  std::error_code ec;
  std::filesystem::remove_all(tmpDir, ec);
}

void TestResolveAssetPathFallsBackToOriginalWhenMissing() {
  const std::string input = "assets/this_file_does_not_exist.xyz";
  const std::string resolved = slam::app::ResolveAssetPath(input);
  ASSERT_TRUE(resolved == input, "resolver must return original path when no candidate exists");
}

void TestResolveAssetPathFromBuildDirectoryFindsProjectAssets() {
  namespace fs = std::filesystem;

  const std::filesystem::path oldCwd = std::filesystem::current_path();
  struct CwdRestore {
    explicit CwdRestore(std::filesystem::path path) : path_(std::move(path)) {}
    ~CwdRestore() {
      std::error_code ec;
      std::filesystem::current_path(path_, ec);
    }
    std::filesystem::path path_;
  } restore(oldCwd);

  fs::path buildDir = oldCwd;
  if (!fs::exists(buildDir / "CMakeCache.txt")) {
    const fs::path nestedBuild = oldCwd / "build";
    if (fs::exists(nestedBuild / "CMakeCache.txt")) {
      buildDir = nestedBuild;
    } else {
      throw std::runtime_error("could not locate build directory from current working directory");
    }
  }

  fs::current_path(buildDir);

  const std::string resolved = slam::app::ResolveAssetPath("assets/maze.png");
  ASSERT_TRUE(std::filesystem::exists(std::filesystem::path(resolved)), "maze path must resolve from build dir");
}

}  // namespace

int main() {
  const std::vector<TestResult> results = {
      Run("Resolve existing relative path", TestResolveAssetPathPrefersExistingRelativePath),
      Run("Resolve missing path fallback", TestResolveAssetPathFallsBackToOriginalWhenMissing),
      Run("Resolve from build directory", TestResolveAssetPathFromBuildDirectoryFindsProjectAssets),
  };

  int failed = 0;
  for (const TestResult& result : results) {
    if (result.passed) {
      std::cout << "[PASS] " << result.name << '\n';
    } else {
      ++failed;
      std::cout << "[FAIL] " << result.name << " :: " << result.message << '\n';
    }
  }
  std::cout << "Total: " << results.size() << ", Failed: " << failed << '\n';
  return failed == 0 ? EXIT_SUCCESS : EXIT_FAILURE;
}

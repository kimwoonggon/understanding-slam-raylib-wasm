/**
 * @file DifferentialTrace.cpp
 * @brief Offline trace runner for differential movement/scan comparisons.
 */

#include <algorithm>
#include <cctype>
#include <cmath>
#include <cstdint>
#include <fstream>
#include <iomanip>
#include <iostream>
#include <sstream>
#include <stdexcept>
#include <string>
#include <utility>
#include <vector>

#include "app/AssetPaths.h"
#include "core/OccupancyGridMap.h"
#include "core/SimulatedLidar.h"
#include "core/Types.h"
#include "input/Motion.h"

namespace {

struct Rgb {
  std::uint8_t r = 0;
  std::uint8_t g = 0;
  std::uint8_t b = 0;
};

constexpr int kWorldWidth = 120;
constexpr int kWorldHeight = 80;
constexpr int kCellSize = 8;
constexpr int kImageWidth = kWorldWidth * kCellSize;
constexpr int kImageHeight = kWorldHeight * kCellSize;
constexpr double kMotionSpeed = 0.5;

constexpr Rgb kMapObstacle{80, 80, 80};
constexpr Rgb kLaser{255, 0, 0};
constexpr Rgb kHitAndRobot{0, 255, 0};

using FrameBuffer = std::vector<std::uint8_t>;

void SetPixel(FrameBuffer& frame, int x, int y, Rgb color) {
  if (x < 0 || x >= kImageWidth || y < 0 || y >= kImageHeight) {
    return;
  }
  const std::size_t index = static_cast<std::size_t>((y * kImageWidth + x) * 3);
  frame[index] = color.r;
  frame[index + 1] = color.g;
  frame[index + 2] = color.b;
}

void DrawRect(FrameBuffer& frame, int x, int y, int width, int height, Rgb color) {
  for (int yy = 0; yy < height; ++yy) {
    for (int xx = 0; xx < width; ++xx) {
      SetPixel(frame, x + xx, y + yy, color);
    }
  }
}

void DrawLine(FrameBuffer& frame, int x0, int y0, int x1, int y1, Rgb color) {
  int dx = std::abs(x1 - x0);
  int dy = std::abs(y1 - y0);
  const int xStep = (x0 < x1) ? 1 : -1;
  const int yStep = (y0 < y1) ? 1 : -1;
  int err = dx - dy;

  while (true) {
    SetPixel(frame, x0, y0, color);
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
}

void DrawFilledCircle(FrameBuffer& frame, int centerX, int centerY, int radius, Rgb color) {
  const int radiusSquared = radius * radius;
  for (int dy = -radius; dy <= radius; ++dy) {
    for (int dx = -radius; dx <= radius; ++dx) {
      if ((dx * dx) + (dy * dy) <= radiusSquared) {
        SetPixel(frame, centerX + dx, centerY + dy, color);
      }
    }
  }
}

std::string Trim(const std::string& input) {
  std::size_t first = 0;
  while (first < input.size() && std::isspace(static_cast<unsigned char>(input[first])) != 0) {
    ++first;
  }
  std::size_t last = input.size();
  while (last > first && std::isspace(static_cast<unsigned char>(input[last - 1])) != 0) {
    --last;
  }
  return input.substr(first, last - first);
}

std::vector<std::string> LoadInputSequence(const std::string& path) {
  std::ifstream file(path);
  if (!file) {
    throw std::runtime_error("failed to open input sequence: " + path);
  }

  std::vector<std::string> sequence;
  std::string line;
  while (std::getline(file, line)) {
    std::string token = Trim(line);
    if (token.rfind("#", 0) == 0) {
      continue;
    }
    for (char& ch : token) {
      ch = static_cast<char>(std::toupper(static_cast<unsigned char>(ch)));
    }
    sequence.push_back(token);
  }

  if (sequence.empty()) {
    throw std::runtime_error("input sequence is empty: " + path);
  }
  return sequence;
}

slam::core::WorldGrid LoadWorldFromGridFile(const std::string& path) {
  std::ifstream file(path);
  if (!file) {
    throw std::runtime_error("failed to open world grid file: " + path);
  }

  slam::core::WorldGrid world(kWorldWidth, kWorldHeight);
  std::string line;
  int y = 0;
  while (std::getline(file, line)) {
    const std::string row = Trim(line);
    if (row.empty()) {
      continue;
    }
    if (y >= kWorldHeight) {
      throw std::runtime_error("world grid has more rows than expected: " + path);
    }
    if (static_cast<int>(row.size()) != kWorldWidth) {
      throw std::runtime_error("world grid row width mismatch at y=" + std::to_string(y));
    }
    for (int x = 0; x < kWorldWidth; ++x) {
      const char cell = row[static_cast<std::size_t>(x)];
      if (cell == '#' || cell == '1') {
        world.SetObstacle(x, y);
      } else if (cell == '.' || cell == '0') {
        continue;
      } else {
        throw std::runtime_error("invalid world grid char at (" + std::to_string(x) + "," +
                                 std::to_string(y) + "): '" + std::string(1, cell) + "'");
      }
    }
    ++y;
  }

  if (y != kWorldHeight) {
    throw std::runtime_error("world grid row count mismatch, expected " + std::to_string(kWorldHeight) +
                             " got " + std::to_string(y));
  }
  return world;
}

std::uint64_t Fnv1a64(const FrameBuffer& frame) {
  std::uint64_t hash = 1469598103934665603ULL;
  for (std::uint8_t byte : frame) {
    hash ^= byte;
    hash *= 1099511628211ULL;
  }
  return hash;
}

int CountChangedPixels(const FrameBuffer& previous, const FrameBuffer& current) {
  const std::size_t byteCount = std::min(previous.size(), current.size());
  int changed = 0;
  for (std::size_t i = 0; i + 2 < byteCount; i += 3) {
    if (previous[i] != current[i] || previous[i + 1] != current[i + 1] || previous[i + 2] != current[i + 2]) {
      ++changed;
    }
  }
  return changed;
}

FrameBuffer RenderSimulationFrame(
    const slam::core::OccupancyGridMap& map,
    const slam::core::RobotPose& pose,
    const std::vector<slam::core::ScanSample>& scan) {
  FrameBuffer frame(static_cast<std::size_t>(kImageWidth * kImageHeight * 3), 0);

  for (int y = 0; y < map.Height(); ++y) {
    for (int x = 0; x < map.Width(); ++x) {
      if (map.ValueAt(x, y) == slam::core::kOccupied) {
        DrawRect(frame, x * kCellSize, y * kCellSize, kCellSize, kCellSize, kMapObstacle);
      }
    }
  }

  const int originX = static_cast<int>(pose.x * static_cast<double>(kCellSize));
  const int originY = static_cast<int>(pose.y * static_cast<double>(kCellSize));
  std::vector<std::pair<int, int>> currentHits;
  currentHits.reserve(scan.size());

  for (const slam::core::ScanSample& sample : scan) {
    const double absoluteAngle = pose.theta + sample.relativeAngle;
    const int endX = static_cast<int>((pose.x + std::cos(absoluteAngle) * sample.distance) * static_cast<double>(kCellSize));
    const int endY = static_cast<int>((pose.y + std::sin(absoluteAngle) * sample.distance) * static_cast<double>(kCellSize));
    DrawLine(frame, originX, originY, endX, endY, kLaser);
    if (sample.hit) {
      currentHits.emplace_back(endX, endY);
    }
  }

  for (const auto& [hitX, hitY] : currentHits) {
    DrawFilledCircle(frame, hitX, hitY, 2, kHitAndRobot);
  }

  DrawRect(
      frame,
      static_cast<int>(pose.x * static_cast<double>(kCellSize)) - 3,
      static_cast<int>(pose.y * static_cast<double>(kCellSize)) - 3,
      6,
      6,
      kHitAndRobot);

  return frame;
}

bool HasKey(const std::string& token, char key) {
  return token.find(key) != std::string::npos;
}

std::string ToHex64(std::uint64_t value) {
  std::ostringstream out;
  out << std::hex << std::nouppercase << std::setw(16) << std::setfill('0') << value;
  return out.str();
}

void PrintUsage(const char* argv0) {
  std::cerr << "Usage: " << argv0 << " --inputs <path> [--world-grid <path>]\n";
}

}  // namespace

int main(int argc, char** argv) {
  try {
    std::string inputsPath;
    std::string worldGridPath = slam::app::ResolveAssetPath("tests/data/world_grid_120x80.txt");

    for (int i = 1; i < argc; ++i) {
      const std::string arg = argv[i];
      if (arg == "--inputs" && i + 1 < argc) {
        inputsPath = argv[++i];
      } else if (arg == "--world-grid" && i + 1 < argc) {
        worldGridPath = argv[++i];
      } else {
        PrintUsage(argv[0]);
        return 2;
      }
    }

    if (inputsPath.empty()) {
      PrintUsage(argv[0]);
      return 2;
    }

    const std::vector<std::string> sequence = LoadInputSequence(inputsPath);

    slam::core::WorldGrid world = LoadWorldFromGridFile(worldGridPath);
    slam::core::OccupancyGridMap map(kWorldWidth, kWorldHeight);
    slam::core::SimulatedLidar lidar(30.0, 72, 1.0);
    slam::core::RobotPose pose{10.0, 10.0, 0.0};

    FrameBuffer previousFrame(static_cast<std::size_t>(kImageWidth * kImageHeight * 3), 0);

    for (std::size_t frameIndex = 0; frameIndex < sequence.size(); ++frameIndex) {
      const std::string& token = sequence[frameIndex];
      const slam::core::RobotPose motionCandidate = slam::input::HandleMotion(
          pose,
          kMotionSpeed,
          HasKey(token, 'W'),
          HasKey(token, 'S'),
          HasKey(token, 'A'),
          HasKey(token, 'D'));

      if (motionCandidate.x != pose.x || motionCandidate.y != pose.y) {
        if (!world.IsObstacle(static_cast<int>(motionCandidate.x), static_cast<int>(motionCandidate.y))) {
          pose = motionCandidate;
        }
      }

      const std::vector<slam::core::ScanSample> scan = lidar.Scan(world, pose);
      map.IntegrateScan(pose, scan);

      const FrameBuffer frame = RenderSimulationFrame(map, pose, scan);
      const int changedPixels = CountChangedPixels(previousFrame, frame);
      const std::uint64_t hash = Fnv1a64(frame);

      std::cout << "{\"frame\":" << frameIndex
                << ",\"hash\":\"" << ToHex64(hash) << "\""
                << ",\"changed\":" << changedPixels
                << ",\"pose\":[" << std::fixed << std::setprecision(6) << pose.x << "," << pose.y << "," << pose.theta
                << "]}\n";

      previousFrame = frame;
    }

    return 0;
  } catch (const std::exception& ex) {
    std::cerr << "error: " << ex.what() << '\n';
    return 1;
  }
}

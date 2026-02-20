#pragma once

#include <functional>

/**
 * @file SoundController.h
 * @brief Audio control abstraction for scan/collision feedback.
 */

namespace slam::audio {

/**
 * @brief Minimal sound interface used for testable audio control.
 */
class SoundLike {
 public:
  virtual ~SoundLike() = default;
  /// Start playback with loop semantics defined by implementation.
  virtual void Play(int loops) = 0;
  /// Stop playback.
  virtual void Stop() = 0;
};

/**
 * @brief Manages scan loop and collision beep playback policy.
 */
class SoundController {
 public:
  /**
   * @brief Construct a controller over externally owned sound objects.
   */
  SoundController(
      SoundLike* scanSound,
      SoundLike* collisionSound,
      bool enabled,
      std::function<double()> timeFn,
      double collisionCooldownSec);

  /// Update scan-loop playback based on current movement activity.
  void UpdateScan(bool active);
  /// Play collision sound with cooldown gating.
  void PlayCollision();
  /// Stop active sounds and reset runtime state.
  void Shutdown();

 private:
  SoundLike* scanSound_ = nullptr;
  SoundLike* collisionSound_ = nullptr;
  bool enabled_ = false;
  std::function<double()> timeFn_;
  double collisionCooldownSec_ = 0.2;
  bool scanPlaying_ = false;
  double lastCollisionTime_ = -10000.0;
};

}  // namespace slam::audio

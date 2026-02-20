#pragma once

#include <functional>

namespace slam::audio {

class SoundLike {
 public:
  virtual ~SoundLike() = default;
  virtual void Play(int loops) = 0;
  virtual void Stop() = 0;
};

class SoundController {
 public:
  SoundController(
      SoundLike* scanSound,
      SoundLike* collisionSound,
      bool enabled,
      std::function<double()> timeFn,
      double collisionCooldownSec);

  void UpdateScan(bool active);
  void PlayCollision();
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

#include "audio/SoundController.h"

#include <utility>

namespace slam::audio {

SoundController::SoundController(
    SoundLike* scanSound,
    SoundLike* collisionSound,
    bool enabled,
    std::function<double()> timeFn,
    double collisionCooldownSec)
    : scanSound_(scanSound),
      collisionSound_(collisionSound),
      enabled_(enabled),
      timeFn_(std::move(timeFn)),
      collisionCooldownSec_(collisionCooldownSec) {}

void SoundController::UpdateScan(bool active) {
  if (!enabled_ || scanSound_ == nullptr) {
    return;
  }

  if (active && !scanPlaying_) {
    scanSound_->Play(-1);
    scanPlaying_ = true;
  } else if (!active && scanPlaying_) {
    scanSound_->Stop();
    scanPlaying_ = false;
  }
}

void SoundController::PlayCollision() {
  if (!enabled_ || collisionSound_ == nullptr) {
    return;
  }

  const double now = timeFn_();
  if (now - lastCollisionTime_ < collisionCooldownSec_) {
    return;
  }

  collisionSound_->Play(0);
  lastCollisionTime_ = now;
}

void SoundController::Shutdown() {
  if (scanSound_ != nullptr) {
    scanSound_->Stop();
  }
  scanPlaying_ = false;
}

}  // namespace slam::audio

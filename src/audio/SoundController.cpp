/**
 * @file SoundController.cpp
 * @brief Audio controller policy implementation for scan/collision sounds.
 */

#include "audio/SoundController.h"

#include <utility>

namespace slam::audio {

/**
 * @brief Construct a sound controller.
 */
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

/**
 * @brief Start/stop scan loop sound based on activity.
 * @param active True when movement/scan activity is present.
 */
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

/**
 * @brief Play collision sound if cooldown allows.
 */
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

/**
 * @brief Stop ongoing scan sound and clear runtime state.
 */
void SoundController::Shutdown() {
  if (scanSound_ != nullptr) {
    scanSound_->Stop();
  }
  scanPlaying_ = false;
}

}  // namespace slam::audio

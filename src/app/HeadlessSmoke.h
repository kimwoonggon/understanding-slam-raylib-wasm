#pragma once

#include "app/Config.h"

/**
 * @file HeadlessSmoke.h
 * @brief Headless smoke execution entrypoint for CI/runtime checks.
 */

namespace slam::app {

/**
 * @brief Execute a deterministic headless simulation loop.
 * @param config Runtime configuration.
 * @param steps Number of simulation steps to execute.
 * @return Process-style exit code.
 */
int RunHeadlessSmoke(const AppConfig& config, int steps);

}  // namespace slam::app

#pragma once

#include <raylib.h>

/**
 * @file UiControls.h
 * @brief UI control rectangle helpers and input predicates.
 */

namespace slam::ui {

/**
 * @brief Screen-space rectangles for app buttons.
 */
struct UiControls {
  /// Reset map control.
  Rectangle reset{};
  /// Toggle world/map view control.
  Rectangle toggleWorld{};
  /// Toggle live/accumulate hit behavior control.
  Rectangle accumulate{};
};

/**
 * @brief Create reset button geometry for split-panel layouts.
 */
Rectangle CreateResetButtonRect(int panelWidth, int windowHeight);
/**
 * @brief Create world-toggle button geometry for split-panel layouts.
 */
Rectangle CreateToggleWorldButtonRect(int panelWidth, int windowHeight);
/**
 * @brief Create accumulate-toggle button geometry for split-panel layouts.
 */
Rectangle CreateAccumulateButtonRect(int panelWidth, int windowHeight);
/**
 * @brief Create a full set of control rectangles for split-panel layouts.
 */
UiControls CreateUiControls(int panelWidth, int windowHeight);
/**
 * @brief Create control rectangles for single-panel window layouts.
 */
UiControls CreateUiControlsForWindow(int windowWidth, int windowHeight);
/**
 * @brief Return true when mouse position is on the reset button.
 */
bool IsResetButtonClick(Vector2 mousePos, const Rectangle& buttonRect);
/**
 * @brief Return true when reset should trigger from keyboard/mouse input.
 */
bool ShouldResetFromInputs(bool iPressed, bool leftClickPressed, Vector2 mousePos, const Rectangle& resetRect);

}  // namespace slam::ui

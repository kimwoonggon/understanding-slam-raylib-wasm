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

/// Create reset button geometry for split-panel layouts.
Rectangle CreateResetButtonRect(int panelWidth, int windowHeight);
/// Create world-toggle button geometry for split-panel layouts.
Rectangle CreateToggleWorldButtonRect(int panelWidth, int windowHeight);
/// Create accumulate-toggle button geometry for split-panel layouts.
Rectangle CreateAccumulateButtonRect(int panelWidth, int windowHeight);
/// Create a full set of control rectangles for split-panel layouts.
UiControls CreateUiControls(int panelWidth, int windowHeight);
/// Create control rectangles for single-panel window layouts.
UiControls CreateUiControlsForWindow(int windowWidth, int windowHeight);
/// Return true when the mouse is on the reset button.
bool IsResetButtonClick(Vector2 mousePos, const Rectangle& buttonRect);
/// Return true when reset should trigger from keyboard/mouse input.
bool ShouldResetFromInputs(bool iPressed, bool leftClickPressed, Vector2 mousePos, const Rectangle& resetRect);

}  // namespace slam::ui

/**
 * @file UiControls.cpp
 * @brief UI button geometry and input predicate implementations.
 */

#include "ui/UiControls.h"

#include <algorithm>

namespace slam::ui {

/**
 * @brief Create reset-button rectangle for split-panel layouts.
 */
Rectangle CreateResetButtonRect(int panelWidth, int windowHeight) {
  constexpr float kButtonWidth = 160.0F;
  constexpr float kButtonHeight = 36.0F;
  constexpr float kMargin = 10.0F;
  const float left = static_cast<float>(panelWidth * 2) - kButtonWidth - kMargin;
  const float top = std::min(kMargin, std::max(0.0F, static_cast<float>(windowHeight) - kButtonHeight - kMargin));
  return Rectangle{left, top, kButtonWidth, kButtonHeight};
}

/**
 * @brief Create world-toggle button rectangle for split-panel layouts.
 */
Rectangle CreateToggleWorldButtonRect(int panelWidth, int windowHeight) {
  constexpr float kButtonWidth = 160.0F;
  constexpr float kButtonHeight = 36.0F;
  constexpr float kMargin = 10.0F;
  const float left = static_cast<float>(panelWidth * 2) - kButtonWidth - kMargin;
  const float top = std::min(
      kMargin + kButtonHeight + 8.0F,
      std::max(0.0F, static_cast<float>(windowHeight) - kButtonHeight - kMargin));
  return Rectangle{left, top, kButtonWidth, kButtonHeight};
}

/**
 * @brief Create accumulate-toggle button rectangle for split-panel layouts.
 */
Rectangle CreateAccumulateButtonRect(int panelWidth, int windowHeight) {
  constexpr float kButtonWidth = 160.0F;
  constexpr float kButtonHeight = 36.0F;
  constexpr float kMargin = 10.0F;
  const float left = static_cast<float>(panelWidth * 2) - kButtonWidth - kMargin;
  const float top = std::min(
      kMargin + (kButtonHeight + 8.0F) * 2.0F,
      std::max(0.0F, static_cast<float>(windowHeight) - kButtonHeight - kMargin));
  return Rectangle{left, top, kButtonWidth, kButtonHeight};
}

/**
 * @brief Create all UI control rectangles for split-panel layout.
 */
UiControls CreateUiControls(int panelWidth, int windowHeight) {
  return UiControls{
      .reset = CreateResetButtonRect(panelWidth, windowHeight),
      .toggleWorld = CreateToggleWorldButtonRect(panelWidth, windowHeight),
      .accumulate = CreateAccumulateButtonRect(panelWidth, windowHeight),
  };
}

/**
 * @brief Create all UI control rectangles for single-window layout.
 */
UiControls CreateUiControlsForWindow(int windowWidth, int windowHeight) {
  constexpr float kButtonWidth = 160.0F;
  constexpr float kButtonHeight = 36.0F;
  constexpr float kMargin = 10.0F;
  (void)windowHeight;
  const float left = static_cast<float>(windowWidth) - kButtonWidth - kMargin;

  return UiControls{
      .reset = Rectangle{left, kMargin, kButtonWidth, kButtonHeight},
      .toggleWorld = Rectangle{left, kMargin + kButtonHeight + 8.0F, kButtonWidth, kButtonHeight},
      .accumulate =
          Rectangle{left, kMargin + (kButtonHeight + 8.0F) * 2.0F, kButtonWidth, kButtonHeight},
  };
}

/**
 * @brief Return whether mouse position hits reset button.
 */
bool IsResetButtonClick(Vector2 mousePos, const Rectangle& buttonRect) {
  return CheckCollisionPointRec(mousePos, buttonRect);
}

/**
 * @brief Evaluate whether reset should be triggered from input state.
 */
bool ShouldResetFromInputs(
    bool iPressed, bool leftClickPressed, Vector2 mousePos, const Rectangle& resetRect) {
  if (iPressed) {
    return true;
  }
  if (leftClickPressed) {
    return IsResetButtonClick(mousePos, resetRect);
  }
  return false;
}

}  // namespace slam::ui

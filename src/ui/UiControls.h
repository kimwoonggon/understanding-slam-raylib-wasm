#pragma once

#include <raylib.h>

namespace slam::ui {

struct UiControls {
  Rectangle reset{};
  Rectangle toggleWorld{};
  Rectangle accumulate{};
};

Rectangle CreateResetButtonRect(int panelWidth, int windowHeight);
Rectangle CreateToggleWorldButtonRect(int panelWidth, int windowHeight);
Rectangle CreateAccumulateButtonRect(int panelWidth, int windowHeight);
UiControls CreateUiControls(int panelWidth, int windowHeight);
UiControls CreateUiControlsForWindow(int windowWidth, int windowHeight);
bool IsResetButtonClick(Vector2 mousePos, const Rectangle& buttonRect);
bool ShouldResetFromInputs(bool iPressed, bool leftClickPressed, Vector2 mousePos, const Rectangle& resetRect);

}  // namespace slam::ui

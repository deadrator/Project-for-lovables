#pragma once
#include "Config.h"
#include "PetLogic.h"

namespace pet {
enum class Screen { Home, Stats, Love };
enum class Action { None, Feed, Play, ToggleSleep };

// Hardware-independent three-button UI. Inputs are true while pressed.
// Actions occur on debounced release, with no auto-repeat while held.
class Controls {
 public:
  Screen screen = Screen::Home;
  uint8_t selection = 0;
  uint32_t loveStarted = 0;

  Action update(bool leftDown, bool selectDown, bool rightDown, uint32_t now) {
    const bool raw[] = {leftDown, selectDown, rightDown};
    int rawCount = 0, stableCount = 0;
    for (int i = 0; i < 3; ++i) {
      buttons_[i].update(raw[i], now);
      rawCount += raw[i];
      stableCount += buttons_[i].down();
    }
    if (screen == Screen::Love && uint32_t(now - loveStarted) >= config::LOVE_DURATION_MS) {
      screen = Screen::Home;
      waitForRelease_ = true;
    }
    // Chords are not commands. Discard all their releases, including staggered
    // ones, so they cannot accidentally move menus or unlock the secret.
    if (rawCount > 1 || stableCount > 1) {
      waitForRelease_ = true;
      secret_.reset();
      secretHold_ = false;
    }
    if (waitForRelease_) {
      if (rawCount == 0 && stableCount == 0) waitForRelease_ = false;
      return Action::None;
    }

    Button& select = buttons_[1];
    if (select.pressed) {
      // Latch at press time: the final hold may finish beyond the tap window.
      secretHold_ = screen == Screen::Stats && secret_.armed(now);
    }
    if (secretHold_ && select.down() && select.held(now) >= config::SECRET_HOLD_MS) {
      screen = Screen::Love;
      loveStarted = now;
      secret_.reset();
      secretHold_ = false;
      waitForRelease_ = true;  // Triggering hold must not dismiss the animation.
      return Action::None;
    }

    for (int i = 0; i < 3; ++i) {
      if (!buttons_[i].released) continue;
      if (screen == Screen::Love) {
        if (i == 1) screen = Screen::Home;
        return Action::None;
      }
      if (screen == Screen::Stats) {
        if (i == 1) {
          screen = Screen::Home;
          secret_.reset();
          secretHold_ = false;
        } else if (buttons_[i].releaseDuration < config::SECRET_TAP_MAX_MS) {
          secret_.tap(i == 0 ? Key::Left : Key::Right, now);
        } else {
          secret_.reset();
        }
        return Action::None;
      }
      if (i == 0) selection = (selection + 3) % 4;
      else if (i == 2) selection = (selection + 1) % 4;
      else if (selection == 3) {
        screen = Screen::Stats;
        secret_.reset();
      } else {
        const Action choices[] = {Action::Feed, Action::Play, Action::ToggleSleep};
        return choices[selection];
      }
    }
    return Action::None;
  }

 private:
  Button buttons_[3];
  Secret secret_;
  bool secretHold_ = false;
  bool waitForRelease_ = false;
};
}  // namespace pet

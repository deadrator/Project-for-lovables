#include <assert.h>
#include <stdint.h>
#include <iostream>
#include "PetLogic.h"
#include "Controls.h"
using pet::Key;
using pet::Screen;
using pet::Action;

struct Harness {
  pet::Controls ui;
  uint32_t now = 0;
  bool raw[3] = {};
  Action tick(uint32_t ms = 0) {
    now += ms;
    return ui.update(raw[0], raw[1], raw[2], now);
  }
  void press(Key key) {
    tick(50);
    raw[static_cast<int>(key)] = true;
    tick(); tick(30);
  }
  Action release(Key key) {
    raw[static_cast<int>(key)] = false;
    tick(); return tick(30);
  }
  Action tap(Key key, uint32_t duration = 100) {
    press(key); tick(duration); return release(key);
  }
  void stats() {
    tap(Key::Left); tap(Key::Select);
    assert(ui.screen == Screen::Stats);
  }
  void code() {
    tap(Key::Left); tap(Key::Right); tap(Key::Left); tap(Key::Right);
  }
  void love() {
    stats(); code(); press(Key::Select); tick(2000);
    assert(ui.screen == Screen::Love);
  }
};

int main() {
  static_assert(config::LEFT_PIN == 32 && config::SELECT_PIN == 33 &&
                config::RIGHT_PIN == 23, "Expected three direct-wired button pins");
  pet::State state;
  for (int i = 0; i < 200; ++i) state.minute();
  assert(state.food == 0 && state.joy == 0 && state.energy == 0);
  assert(!state.play());
  assert(state.feed() && state.food == 15);
  for (int i = 0; i < 20; ++i) state.feed();
  assert(state.food == 100);
  state.sleeping = true;
  assert(!state.feed() && !state.play());
  for (int i = 0; i < 30; ++i) state.minute();
  assert(state.energy == 100 && state.food == 70 && state.joy == 0);
  state.sleeping = false;
  assert(state.play());
  assert(state.energy == 92 && state.joy == 15 && state.food == 67);
  state.food = 1;
  state.play();
  assert(state.food == 0);
  state.joy = 99;
  state.play();
  assert(state.joy == 100);

  pet::Secret secret;
  assert(!secret.armed(0));
  secret.tap(Key::Left, 100); secret.tap(Key::Right, 300);
  secret.tap(Key::Left, 500);
  assert(!secret.armed(500));
  secret.tap(Key::Right, 700);
  assert(secret.armed(5700));
  assert(!secret.armed(5701));
  secret.tap(Key::Left, 6000); secret.tap(Key::Left, 6200); // Wrong order.
  secret.tap(Key::Right, 6400); secret.tap(Key::Left, 6600);
  assert(!secret.armed(6600));
  secret.reset();
  secret.tap(Key::Left, 7000); secret.tap(Key::Right, 7200);
  secret.tap(Key::Left, 7400); secret.tap(Key::Right, 7600);
  secret.tap(Key::Left, 7800); // Extra input cancels, does not restart.
  assert(!secret.armed(7800));
  secret.tap(Key::Left, 8000); secret.tap(Key::Right, 14000);
  secret.tap(Key::Left, 14200); secret.tap(Key::Right, 14400);
  assert(!secret.armed(14400));
  secret.reset();
  secret.tap(Key::Left, UINT32_MAX - 500);
  secret.tap(Key::Right, UINT32_MAX - 300);
  secret.tap(Key::Left, UINT32_MAX - 100);
  secret.tap(Key::Right, 100);
  assert(secret.armed(300));
  assert(!secret.armed(5101));

  pet::Button button;
  button.update(true, 100);
  button.update(false, 110);  // Bounce ignored.
  button.update(true, 120);
  button.update(true, 149);
  assert(!button.down() && !button.pressed);
  button.update(true, 150);
  assert(button.down() && button.pressed);
  button.update(true, 200);
  assert(!button.pressed && button.held(200) == 50);
  button.update(false, 250);
  button.update(false, 280);
  assert(button.released && !button.down() && button.releaseDuration == 130);
  button.update(false, 300);
  assert(!button.released);
  button.update(true, 1000);
  button.update(true, 1030);
  button.update(true, 3030);
  assert(button.held(3030) == 2000);
  button.update(false, 3100);
  button.update(false, 3130);
  assert(button.releaseDuration == 2100);

  pet::Button wrapButton;
  wrapButton.update(true, UINT32_MAX - 100);
  wrapButton.update(true, UINT32_MAX - 70);
  wrapButton.update(false, 100);
  wrapButton.update(false, 130);
  assert(wrapButton.released && wrapButton.releaseDuration == 201);

  Harness menu;
  assert(menu.tap(Key::Select) == Action::Feed);
  menu.tap(Key::Right);
  assert(menu.ui.selection == 1);
  assert(menu.tap(Key::Select) == Action::Play);
  menu.tap(Key::Right);
  assert(menu.tap(Key::Select) == Action::ToggleSleep);
  menu.tap(Key::Right); menu.tap(Key::Right);
  assert(menu.ui.selection == 0); // Wrap right.
  menu.tap(Key::Left);
  assert(menu.ui.selection == 3); // Wrap left.
  menu.tap(Key::Select);
  assert(menu.ui.screen == Screen::Stats);
  menu.tap(Key::Select);
  assert(menu.ui.screen == Screen::Home);

  Harness happy;
  happy.stats(); happy.code(); happy.press(Key::Select);
  happy.tick(1999);
  assert(happy.ui.screen == Screen::Stats);
  happy.tick(1);
  assert(happy.ui.screen == Screen::Love);
  happy.release(Key::Select);
  assert(happy.ui.screen == Screen::Love); // Triggering release is swallowed.
  happy.tap(Key::Left); happy.tap(Key::Right);
  assert(happy.ui.screen == Screen::Love);
  happy.tap(Key::Select);
  assert(happy.ui.screen == Screen::Home);

  Harness automatic;
  automatic.love(); automatic.release(Key::Select);
  automatic.tick(config::LOVE_DURATION_MS);
  assert(automatic.ui.screen == Screen::Home);

  Harness held;
  held.love(); held.tick(config::LOVE_DURATION_MS);
  assert(held.ui.screen == Screen::Home);
  held.release(Key::Select);
  assert(held.ui.screen == Screen::Home); // Must not reopen Stats.

  Harness incomplete;
  incomplete.stats(); incomplete.tap(Key::Left); incomplete.tap(Key::Right);
  incomplete.tap(Key::Select, 2500);
  assert(incomplete.ui.screen == Screen::Home);

  Harness wrongOrder;
  wrongOrder.stats(); wrongOrder.tap(Key::Right); wrongOrder.tap(Key::Left);
  wrongOrder.tap(Key::Right); wrongOrder.tap(Key::Left);
  wrongOrder.tap(Key::Select, 2500);
  assert(wrongOrder.ui.screen == Screen::Home);

  Harness tooShort;
  tooShort.stats(); tooShort.code(); tooShort.tap(Key::Select, 1500);
  assert(tooShort.ui.screen == Screen::Home);

  Harness expired;
  expired.stats(); expired.code(); expired.tick(5001);
  expired.tap(Key::Select, 2500);
  assert(expired.ui.screen == Screen::Home);

  Harness spanningWindow;
  spanningWindow.stats(); spanningWindow.code(); spanningWindow.tick(4500);
  spanningWindow.press(Key::Select); spanningWindow.tick(2000);
  assert(spanningWindow.ui.screen == Screen::Love);

  Harness outside;
  outside.code(); // Left/right navigation on Home must not count.
  assert(outside.tap(Key::Select, 2500) == Action::Feed);
  assert(outside.ui.screen == Screen::Home);

  Harness longDirection;
  longDirection.stats(); longDirection.tap(Key::Left, 1000);
  longDirection.tap(Key::Right); longDirection.tap(Key::Left); longDirection.tap(Key::Right);
  longDirection.tap(Key::Select, 2500);
  assert(longDirection.ui.screen == Screen::Home);

  Harness repeat;
  repeat.press(Key::Right); repeat.tick(10000);
  assert(repeat.ui.selection == 0); // Actions only on release, no auto-repeat.
  repeat.release(Key::Right);
  assert(repeat.ui.selection == 1);

  Harness chords;
  chords.raw[0] = chords.raw[2] = true; chords.tick(); chords.tick(30);
  chords.release(Key::Left); chords.release(Key::Right);
  assert(chords.ui.selection == 0);
  chords.tap(Key::Right);
  assert(chords.ui.selection == 1); // Recovers after all buttons released.

  Harness interrupted;
  interrupted.stats(); interrupted.code(); interrupted.press(Key::Select);
  interrupted.tick(1000); interrupted.press(Key::Left); interrupted.tick(2000);
  assert(interrupted.ui.screen == Screen::Stats); // Chord cancels armed hold.
  interrupted.release(Key::Select); interrupted.release(Key::Left);
  interrupted.tap(Key::Select, 2500);
  assert(interrupted.ui.screen == Screen::Home);

  Harness resetOnExit;
  resetOnExit.stats(); resetOnExit.code(); resetOnExit.tap(Key::Select);
  resetOnExit.tap(Key::Select); // Reopen Stats without changing Home selection.
  resetOnExit.tap(Key::Select, 2500);
  assert(resetOnExit.ui.screen == Screen::Home);

  Harness wrap;
  wrap.now = UINT32_MAX - 8000;
  wrap.love(); wrap.release(Key::Select); wrap.tick(config::LOVE_DURATION_MS);
  assert(wrap.ui.screen == Screen::Home);

  std::cout << "All pet, debounce, three-button navigation and secret-sequence tests passed.\n";
}

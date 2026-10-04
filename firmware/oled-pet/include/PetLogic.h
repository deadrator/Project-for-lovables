#pragma once
#include <stdint.h>

namespace pet {
inline uint8_t clamp(int value) {
  return static_cast<uint8_t>(value < 0 ? 0 : (value > 100 ? 100 : value));
}

struct State {
  uint8_t food = 80;
  uint8_t joy = 80;
  uint8_t energy = 80;
  bool sleeping = false;

  void minute() {
    food = clamp(food - 1);
    joy = clamp(joy - (sleeping ? 0 : 1));
    energy = clamp(energy + (sleeping ? 4 : -1));
  }
  bool feed() {
    if (sleeping) return false;
    food = clamp(food + 15);
    return true;
  }
  bool play() {
    if (sleeping || energy < 10) return false;
    joy = clamp(joy + 15);
    energy = clamp(energy - 8);
    food = clamp(food - 3);
    return true;
  }
};

enum class Key : uint8_t { Left, Select, Right };

// LEFT, RIGHT, LEFT, RIGHT short releases; then hold SELECT in Stats.
// Wrong/extra input or a gap greater than five seconds clears the sequence.
class Secret {
 public:
  void reset() { progress_ = 0; }
  void expire(uint32_t now) {
    if (progress_ && uint32_t(now - lastTap_) > 5000) reset();
  }
  void tap(Key key, uint32_t now) {
    expire(now);
    const Key sequence[] = {Key::Left, Key::Right, Key::Left, Key::Right};
    if (progress_ < 4 && key == sequence[progress_]) ++progress_;
    else reset();
    lastTap_ = now;
  }
  bool armed(uint32_t now) {
    expire(now);
    return progress_ == 4;
  }
 private:
  uint8_t progress_ = 0;
  uint32_t lastTap_ = 0;
};

// Active-high logical 'down' input. GPIO inversion is done by the caller.
// Unsigned timestamp subtraction also works across millis() rollover.
class Button {
 public:
  bool pressed = false;
  bool released = false;
  uint32_t releaseDuration = 0;

  void update(bool rawDown, uint32_t now) {
    pressed = released = false;
    if (rawDown != raw_) {
      raw_ = rawDown;
      changedAt_ = now;
    }
    if (raw_ != stable_ && uint32_t(now - changedAt_) >= 30) {
      stable_ = raw_;
      if (stable_) {
        pressed = true;
        pressedAt_ = now;
      } else {
        released = true;
        releaseDuration = uint32_t(now - pressedAt_);
      }
    }
  }
  bool down() const { return stable_; }
  uint32_t held(uint32_t now) const {
    return stable_ ? uint32_t(now - pressedAt_) : 0;
  }
 private:
  bool raw_ = false;
  bool stable_ = false;
  uint32_t changedAt_ = 0;
  uint32_t pressedAt_ = 0;
};
}  // namespace pet

#pragma once
#include <stdint.h>

namespace config {
constexpr uint8_t OLED_ADDRESS = 0x3C;  // Some modules use 0x3D.
constexpr int SDA_PIN = 21;
constexpr int SCL_PIN = 22;
// Three normally-open momentary buttons, each connected to GND.
// All use INPUT_PULLUP; these pins are not boot-strapping pins.
constexpr int LEFT_PIN = 32;
constexpr int SELECT_PIN = 33;
constexpr int RIGHT_PIN = 23;
constexpr uint32_t SECRET_TAP_MAX_MS = 600;
constexpr uint32_t SECRET_HOLD_MS = 2000;
constexpr uint32_t LOVE_DURATION_MS = 12000;
constexpr char LOVE_MESSAGE[] = "I LOVE YOU";  // Keep short for a 128px display.

// Leave disabled until you identify your LED. This driver is ONLY for a
// low-current, bare four-leg RGB LED with one series resistor per color.
// It is NOT a WS2812/NeoPixel driver. No LED wiring is needed by default.
constexpr bool RGB_ENABLED = false;
constexpr bool RGB_COMMON_ANODE = false;
constexpr int RED_PIN = 25;
constexpr int GREEN_PIN = 26;
constexpr int BLUE_PIN = 27;
}  // namespace config

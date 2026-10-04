# Project-for-lovables

## Buddy: ESP32 OLED pet

The new [OLED pet firmware and wiring guide](firmware/oled-pet/README.md) targets a classic ESP32 with 4 MB flash and a white 128×64 SSD1306 I²C OLED.

Features:
- Animated pet with feed, play, sleep, and stats menus.
- Three-button navigation: Left (GPIO32), Select (GPIO33), Right (GPIO23), each wired directly to GND.
- Hidden **Left → Right → Left → Right → hold Select for two seconds inside Stats** to reveal a beating heart and **I LOVE YOU**.
- Saved pet stats and optional resistor-protected RGB status LED.
- No Wi-Fi, speaker, or original Tamagotchi ROM required; three plain pushbuttons are used, with no button module or GPIO expander.

Start with USB power. Battery wiring depends on your exact board and charging module and is not assumed.

**Validation:** hardware-independent tests pass. ESP32 compilation was attempted but blocked by access to the PlatformIO package registry; physical hardware has not been tested. See the guide for build/upload steps and the validation checklist.

The existing `modusec.py` is unchanged and unrelated to this firmware.

# Buddy — OLED pet with a secret heart

An original, small Arduino/PlatformIO starter for a **classic ESP32-D0WD-V3, 4 MB flash**, a **128×64 SSD1306 I²C white OLED**, and three **external normally-open momentary pushbuttons: Left, Select, and Right**. No speaker, Wi-Fi, Bluetooth, ROM, or custom PCB is needed. The onboard BOOT button is not used for gameplay.

This is not a port of either linked repository and is not a Tamagotchi P1 emulator. It implements a simple pet inspired by the idea: feeding, play, sleep, stats, and a hidden affection animation. “Play” raises joy and shows feedback; it is not a mini-game yet.

## 1. Wire the OLED — test on USB first

Disconnect power while wiring. This assumes a development board with USB, an onboard regulator, and exposed GPIO32, GPIO33, and GPIO23 pins, not a bare ESP32 chip. The chip identification alone does not identify your board's power circuit.

| OLED pin | ESP32 development-board pin |
| --- | --- |
| GND | GND |
| VCC | **3V3** |
| SDA | GPIO **21** |
| SCL / SCK (I²C clock) | GPIO **22** |

Use a 3.3 V-compatible **four-pin I²C SSD1306 module**. Follow the printed pin labels, not a photo's pin order. Do not connect 5 V pull-ups to the ESP32's GPIOs. SDA/SCL pins here are GPIO numbers, not physical header positions.

The default I²C address is `0x3C`. Change `OLED_ADDRESS` in `include/Config.h` to `0x3D` if your module uses that address. A SH1106, 128×32 screen, or SPI display needs a different configuration/driver.

### Three navigation buttons — no module required

Use three **plain normally-open momentary pushbuttons**, wired directly:

```text
GPIO32 ───── LEFT button ───── GND
GPIO33 ──── SELECT button ──── GND
GPIO23 ───── RIGHT button ──── GND
```

All three use the ESP32's internal `INPUT_PULLUP`: HIGH when released, LOW when pressed. **No button module, GPIO expander, touch controller, or external pull-up resistors are needed** for short wires. These GPIOs are not boot-strapping pins and do not overlap the OLED or optional RGB pins. Do not connect buttons to 5 V or VIN.

For four-leg tactile switches, the legs usually form two internally connected pairs. Use one terminal from each pair so the circuit is open when released and closes only when pressed. Confirm with continuity testing. Keep wires short; all GND connections share the same ground.

The onboard BOOT and EN/RESET buttons are not gameplay controls.

### Minimal hardware list

- Your existing ESP32 development board.
- White 128×64 I²C SSD1306 OLED.
- **Three ordinary tactile pushbuttons** and connecting wires.
- Your RGB LED; if it is a bare four-leg LED, three current-limiting resistors (see below).
- Battery and a suitable charging/power solution once identified (see battery section).

**No speaker, buzzer, RTC, SD card, sensor board, or other feature module.** The pet and secret animation work on USB with just the OLED and three buttons; RGB and battery are not needed for that first test. Safe battery regulation/protection is still necessary, not an optional feature module.

## 2. Build and upload from your computer

1. Install VS Code and the **PlatformIO IDE** extension.
2. Open **this folder** (`firmware/oled-pet`), the one containing `platformio.ini`.
3. Connect your ESP32 to your computer with a USB data cable.
4. Use **PlatformIO: Build**, then **PlatformIO: Upload**.
5. Use **PlatformIO: Serial Monitor** at **115200 baud** for diagnostics.

Or, with PlatformIO CLI installed:

```sh
cd firmware/oled-pet
pio run
pio run --target upload
pio device monitor --baud 115200
```

The configuration targets generic `esp32dev` with 4 MB flash, not ESP32-S3. It pins Espressif32 platform `6.10.0` (Arduino core 2.x), Adafruit GFX `1.11.11`, and Adafruit SSD1306 `2.5.13`. The optional RGB driver uses Arduino 2.x LEDC APIs; do not switch to Arduino 3.x without updating those calls.

PlatformIO downloads dependencies on the first build. If upload cannot connect, confirm the USB cable, selected serial port, and CH340 driver. Some boards need BOOT held briefly during the upload's “Connecting” stage; release it once flashing begins. If uploads are unreliable, lower `upload_speed` to `115200`.

**During normal use, don't hold BOOT while powering on or resetting.** GPIO0 is a boot-strapping pin and holding it low at reset enters the ROM downloader instead of the pet. Use the three external buttons for controls, not BOOT or EN/RESET.

## 3. Everyday controls

| Where | Button | Result |
| --- | --- | --- |
| Home | Left | Previous menu item, wrapping around |
| Home | Right | Next menu item, wrapping around |
| Home | Select | Activate the highlighted item |
| Stats | Select | Back to home (except when completing the secret hold) |
| Stats | Left / Right | Enter the hidden sequence; otherwise stay on Stats |
| Heart animation | Select | Back to home early |
| Heart animation | Left / Right | No action |

Ordinary navigation uses **press and release**, not long presses. Buttons are debounced in software; holding one does not repeat actions. Press one at a time: simultaneous/overlapping presses are ignored until all buttons are released and cancel any secret sequence.

Menus: **FEED → PLAY → SLEEP/WAKE → STATS**. The starting selection is FEED. From FEED, one Left press wraps to STATS, then Select opens it. The Stats screen shows `SELECT: back`.

- Feeding restores food/fullness. Higher values are better.
- Playing restores joy and uses a little food and energy.
- A sleeping pet cannot eat or play; choose WAKE first.
- Every minute awake, food, joy, and energy decrease by one.
- Every minute asleep, food decreases by one, joy stays unchanged, and energy increases by four.
- Stats stay between 0 and 100. There is no death mechanic in this starter.

## 4. The secret “I LOVE YOU” sequence

1. Navigate to **STATS** using Left/Right, then press and release **Select**.
2. While on Stats, tap **Left → Right → Left → Right**, releasing each button before the next. Each tap must be shorter than 0.6 seconds, with gaps no longer than five seconds.
3. Within five seconds of the final Right release, press and **hold Select for two seconds**.
4. The pet closes its eyes, a little heart grows into a beating heart, floating hearts rise around it, and **I LOVE YOU** appears below.
5. It returns home automatically after **12 seconds**. Releasing the triggering Select hold does not dismiss it; a new Select press/release exits early.

The secret works **only inside Stats**. A wrong or extra direction, a long direction press, overlapping buttons, or a gap over five seconds clears it. Releasing Select before completing the secret exits Stats and clears the sequence. The two-second hold may finish after the five-second window, as long as it started within it. It can be repeated any time; there is no destructive reset or additional hidden hardware.

The animation is **silent**. A monochrome OLED cannot show pink; its hearts are white. Only the optional RGB LED pulses pink.

### Change the message

Edit `include/Config.h`:

```cpp
constexpr char LOVE_MESSAGE[] = "I LOVE YOU";
constexpr uint32_t LOVE_DURATION_MS = 12000;
```

Use short plain ASCII text (at most 21 characters at the default text size). For example, `"I LOVE YOU, MAYA"`. The default display font does not support emoji or arbitrary Unicode. Rebuild and upload after changing it.

## 5. Optional RGB LED

**Disabled by default.** Identify the LED before connecting it. This code supports a **bare four-leg, low-current RGB LED**, not a WS2812/NeoPixel, an unspecified RGB module, or a high-power LED.

For a **common-cathode** LED:

| ESP32 | Connection |
| --- | --- |
| GPIO25 | **470 Ω series resistor** → red anode |
| GPIO26 | **470 Ω series resistor** → green anode |
| GPIO27 | **470 Ω series resistor** → blue anode |
| GND | Common cathode |

Use **one resistor per color**, not a single resistor on the common lead. Confirm the lead order from the LED datasheet. 470 Ω is a conservative starting value for a small indicator LED on 3.3 V; brightness varies by LED. If changing it, calculate current using `(3.3 V − LED forward voltage) / resistance` and keep each color to a few mA.

Set `RGB_ENABLED = true` in `include/Config.h`. For a common-anode LED, connect the common anode to **3V3**, use the same three resistors on the color cathodes, and set `RGB_COMMON_ANODE = true`. Never put 5 V on the common anode in this direct-GPIO circuit.

Colors: dim green normally, amber when a stat is low, blue while the pet sleeps, and pulsing pink during the secret. Brightness is intentionally limited.

## 6. Battery and charging — not wired yet

**Keep using USB until the exact development board, battery, and charger are identified.** Battery wiring is intentionally not guessed here.

- A nominal 3.7 V Li-ion/LiPo cell reaches **4.2 V** fully charged. **Never connect it directly to 3V3.**
- A plain TP4056 is a charger, not a regulated 3.3 V/5 V supply. Some boards include cell protection, but that is not voltage regulation or load sharing.
- Feeding a single cell into an arbitrary board's VIN/5V input can brown out as the cell discharges because of the onboard regulator's dropout.
- A portable design needs suitable cell protection, a charge current appropriate to the battery, and a regulator/power path matched to the actual ESP32 board. Use a proper power-path/load-sharing design if it should operate while charging.
- Do not connect USB and an external power supply together without checking the board's power isolation/backfeed protection.
- No battery-level input or percentage is implemented; estimating it correctly needs additional circuitry and calibration.

Wi-Fi/Bluetooth are not started, but the CPU and OLED stay on. **The pet's SLEEP command is a game state, not ESP32 deep sleep.** The USB bridge, power LED, and regulator also consume power. This is not yet a low-power battery-optimized design and no battery-life estimate is promised.

## 7. Saving and limitations

Stats and sleep state are saved as one versioned NVS blob, at most once per minute when changes are pending. A power cut can lose roughly the last minute of progress. The secret sequence is never saved. Invalid stored values are ignored.

Time only advances while powered. There is no external RTC or network clock, so no offline aging or real day/night cycle. Wi-Fi, Bluetooth, a phone UI, audio, touch pads, and original Tamagotchi ROM emulation are intentionally absent.

To reset progress deliberately, a full flash erase (`pio run --target erase`) clears the saved state and **all other flash contents**, including the firmware; upload again afterward. Normal gameplay has no erase shortcut.

## 8. Validation status

- **Passed:** host C++ tests for stat clamping, feeding, play, sleep, debounce, timestamp rollover, secret direction order/expiry, three-button menu flow, chord rejection, exact two-second hold, release suppression, and automatic return from the animation.
- **Not verified:** ESP32-target compilation. The attempted `pio run` was blocked while downloading Espressif32 because the environment could not reach `api.registry.platformio.org` (`HTTPClientError`; HTTPS probe failed). No flashable binary is supplied or claimed tested.
- **Not verified:** physical OLED, three external pushbuttons, RGB LED, power circuit, and battery runtime. No ESP32 is attached to this environment.

Run the host tests on a machine with a C++ compiler:

```sh
cd firmware/oled-pet
g++ -std=c++11 -Wall -Wextra -Werror -pedantic -Iinclude \
  tests/test_logic.cpp -o /tmp/oled-pet-tests
/tmp/oled-pet-tests
```

### Hardware acceptance checklist

- [ ] `pio run` builds successfully on your computer.
- [ ] USB-powered OLED shows Buddy and all four menu labels.
- [ ] Left (GPIO32) and Right (GPIO23) move the highlight both ways, including wraparound.
- [ ] Select (GPIO33) activates the highlighted item and returns from Stats.
- [ ] All three released inputs are HIGH; BOOT is not used for gameplay.
- [ ] Left → Right → Left → Right → hold Select two seconds **inside Stats** triggers the white heart and message.
- [ ] Wrong order, expired steps, long direction presses, or button chords do not unlock it.
- [ ] Releasing the secret hold leaves the animation visible; it returns after 12 seconds.
- [ ] After waiting over a minute, a reset restores saved stats.
- [ ] If fitted and enabled, the resistor-protected RGB LED uses the correct polarity and colors.
- [ ] Battery wiring is reviewed against the actual board/charger before connecting it.

If the OLED is blank, check the serial diagnostic, common ground, SDA/SCL labels, controller type and I²C address. Disconnect power before changing wires. If needed, change `display.dim(true)` to `display.dim(false)` in `src/main.cpp` for normal contrast.

## Project layout

- `src/main.cpp` — SSD1306 drawings, heart animation, hardware I/O, optional RGB, NVS saving.
- `include/Controls.h` — tested three-button menu and secret-trigger state machine.
- `include/PetLogic.h` — tested pet stats, debounce, and secret-sequence timing.
- `include/Config.h` — pins, LED polarity, secret text and timing.
- `platformio.ini` — ESP32 build configuration and library versions.
- `tests/test_logic.cpp` — hardware-independent tests.

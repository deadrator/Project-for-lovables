#include <Arduino.h>
#include <Wire.h>
#include <Adafruit_GFX.h>
#include <Adafruit_SSD1306.h>
#include <Preferences.h>
#include <math.h>
#include "Config.h"
#include "PetLogic.h"
#include "Controls.h"

namespace {
Adafruit_SSD1306 display(128, 64, &Wire, -1);
Preferences preferences;
pet::State buddy;
pet::Controls controls;
using pet::Screen;
bool displayReady = false;
bool storageReady = false;
bool dirty = false;
uint32_t lastMinute = 0;
uint32_t lastFrame = 0;
uint32_t lastSave = 0;
uint32_t toastStarted = 0;
const char* toast = nullptr;

void say(const char* text, uint32_t now) {
  toast = text;
  toastStarted = now;
}

void saveState(uint32_t now) {
  // One versioned blob; rate-limited to at most one save per minute.
  // Avoid multiple NVS writes per animation frame or per button press.
  if (storageReady && dirty && uint32_t(now - lastSave) >= 60000) {
    const uint8_t bytes[] = {1, buddy.food, buddy.joy, buddy.energy,
                             static_cast<uint8_t>(buddy.sleeping)};
    if (preferences.putBytes("state", bytes, sizeof(bytes)) == sizeof(bytes)) {
      dirty = false;
    }
    lastSave = now;
  }
}

void loadState() {
  storageReady = preferences.begin("oled-pet", false);
  if (!storageReady) {
    Serial.println("NVS unavailable; pet will run without saving.");
    return;
  }
  uint8_t bytes[5] = {};
  if (preferences.getBytesLength("state") == sizeof(bytes) &&
      preferences.getBytes("state", bytes, sizeof(bytes)) == sizeof(bytes) &&
      bytes[0] == 1 && bytes[1] <= 100 && bytes[2] <= 100 &&
      bytes[3] <= 100 && bytes[4] <= 1) {
    buddy.food = bytes[1];
    buddy.joy = bytes[2];
    buddy.energy = bytes[3];
    buddy.sleeping = bytes[4] != 0;
  }
}

void center(const char* text, int y, uint8_t size = 1) {
  int16_t x1, y1;
  uint16_t width, height;
  display.setTextSize(size);
  display.getTextBounds(text, 0, y, &x1, &y1, &width, &height);
  display.setCursor((128 - static_cast<int>(width)) / 2, y);
  display.print(text);
  display.setTextSize(1);
}

void heart(int x, int y, int radius) {
  display.fillCircle(x - radius, y, radius, SSD1306_WHITE);
  display.fillCircle(x + radius, y, radius, SSD1306_WHITE);
  display.fillTriangle(x - 2 * radius, y, x + 2 * radius, y,
                       x, y + 3 * radius, SSD1306_WHITE);
}

void drawBuddy(uint32_t now, bool closeEyes = false) {
  int bob = buddy.sleeping ? 0 : static_cast<int>(sinf(now / 320.0f) * 2);
  int y = 22 + bob;
  display.fillRoundRect(44, y, 40, 26, 10, SSD1306_WHITE);
  display.fillTriangle(45, y + 8, 47, y - 6, 58, y + 3, SSD1306_WHITE);
  display.fillTriangle(70, y + 3, 81, y - 6, 83, y + 8, SSD1306_WHITE);
  bool blink = buddy.sleeping || closeEyes || (now % 4000 < 180);
  if (blink) {
    display.drawFastHLine(53, y + 9, 6, SSD1306_BLACK);
    display.drawFastHLine(69, y + 9, 6, SSD1306_BLACK);
  } else {
    display.fillRect(55, y + 7, 3, 5, SSD1306_BLACK);
    display.fillRect(70, y + 7, 3, 5, SSD1306_BLACK);
  }
  if (buddy.food < 25 || buddy.joy < 25) {
    display.drawLine(61, y + 19, 64, y + 17, SSD1306_BLACK);
    display.drawLine(64, y + 17, 67, y + 19, SSD1306_BLACK);
  } else {
    display.drawLine(61, y + 16, 64, y + 18, SSD1306_BLACK);
    display.drawLine(64, y + 18, 67, y + 16, SSD1306_BLACK);
  }
  display.fillRoundRect(46, y + 23, 12, 4, 2, SSD1306_WHITE);
  display.fillRoundRect(70, y + 23, 12, 4, 2, SSD1306_WHITE);
  if (buddy.sleeping) {
    display.setCursor(92, 22);
    display.print("z Z");
  }
}

void meter(const char* label, uint8_t value, int y) {
  display.setCursor(3, y);
  display.print(label);
  display.drawRect(43, y, 58, 7, SSD1306_WHITE);
  display.fillRect(44, y + 1, 56 * value / 100, 5, SSD1306_WHITE);
  display.setCursor(106, y);
  display.print(value);
}

void render(uint32_t now) {
  if (!displayReady) return;
  display.clearDisplay();
  display.setTextColor(SSD1306_WHITE);
  display.setTextSize(1);
  if (controls.screen == Screen::Love) {
    uint32_t elapsed = uint32_t(now - controls.loveStarted);
    if (elapsed < 1000) {
      center("A little secret...", 2);
      drawBuddy(now, true);
      heart(95, 34, 2);
    } else {
      uint32_t t = elapsed - 1000;
      float pulse = powf((sinf(t / 170.0f) + 1) / 2, 3);
      int radius = t < 800 ? 2 + t * 6 / 800 : 8 + static_cast<int>(2 * pulse);
      heart(64, 19, radius);
      for (int i = 0; i < 4; ++i) {
        int x = i < 2 ? 12 + i * 15 : 98 + (i - 2) * 15;
        int y = 45 - static_cast<int>((t / 65 + i * 13) % 48);
        heart(x, y, 2);
      }
      center(config::LOVE_MESSAGE, 54);
    }
  } else if (controls.screen == Screen::Stats) {
    center("BUDDY / STATS", 0);
    display.drawFastHLine(0, 10, 128, SSD1306_WHITE);
    meter("Food", buddy.food, 16);
    meter("Joy", buddy.joy, 29);
    meter("Rest", buddy.energy, 42);
    center("SELECT: back", 56);
  } else {
    center(toast && uint32_t(now - toastStarted) < 1800 ? toast :
           (buddy.sleeping ? "BUDDY / SLEEPING" : "BUDDY / AWAKE"), 0);
    drawBuddy(now);
    const char* labels[] = {"FEED", "PLAY", buddy.sleeping ? "WAKE" : "SLEEP", "STATS"};
    for (int i = 0; i < 4; ++i) {
      if (controls.selection == i) display.fillRect(i * 32, 54, 32, 10, SSD1306_WHITE);
      display.setTextColor(controls.selection == i ? SSD1306_BLACK : SSD1306_WHITE);
      display.setCursor(i * 32 + (32 - strlen(labels[i]) * 6) / 2, 56);
      display.print(labels[i]);
    }
  }
  display.display();
}

void rgb(uint32_t now) {
  if (!config::RGB_ENABLED) return;
  uint8_t r = 0, g = 0, b = 0;
  if (controls.screen == Screen::Love) {
    float pulse = (sinf(uint32_t(now - controls.loveStarted) / 170.0f) + 1) / 2;
    r = 10 + static_cast<uint8_t>(30 * pulse);
    b = r / 3;
  } else if (buddy.sleeping) {
    b = 12;
  } else if (buddy.food < 25 || buddy.joy < 25 || buddy.energy < 25) {
    r = 24; g = 5;
  } else {
    g = 12;
  }
  const uint8_t levels[] = {r, g, b};
  for (int i = 0; i < 3; ++i) {
    ledcWrite(i, config::RGB_COMMON_ANODE ? 255 - levels[i] : levels[i]);
  }
}

void input(uint32_t now) {
  switch (controls.update(digitalRead(config::LEFT_PIN) == LOW,
                          digitalRead(config::SELECT_PIN) == LOW,
                          digitalRead(config::RIGHT_PIN) == LOW, now)) {
    case pet::Action::Feed:
      if (buddy.feed()) { dirty = true; say("Yum! Thank you.", now); }
      else say("Wake me first!", now);
      break;
    case pet::Action::Play:
      if (buddy.play()) { dirty = true; say("That was fun!", now); }
      else say(buddy.sleeping ? "Wake me first!" : "I need some rest!", now);
      break;
    case pet::Action::ToggleSleep:
      buddy.sleeping = !buddy.sleeping;
      dirty = true;
      break;
    case pet::Action::None:
      break;
  }
}
}  // namespace

void setup() {
  Serial.begin(115200);
  pinMode(config::LEFT_PIN, INPUT_PULLUP);
  pinMode(config::SELECT_PIN, INPUT_PULLUP);
  pinMode(config::RIGHT_PIN, INPUT_PULLUP);
  // Wi-Fi and Bluetooth are deliberately not started.
  if (config::RGB_ENABLED) {
    const int pins[] = {config::RED_PIN, config::GREEN_PIN, config::BLUE_PIN};
    for (int i = 0; i < 3; ++i) {
      pinMode(pins[i], OUTPUT);
      digitalWrite(pins[i], config::RGB_COMMON_ANODE ? HIGH : LOW);
      ledcSetup(i, 5000, 8);
      ledcWrite(i, config::RGB_COMMON_ANODE ? 255 : 0);
      ledcAttachPin(pins[i], i);
    }
  }
  Wire.begin(config::SDA_PIN, config::SCL_PIN);
  // SSD1306 begin() does not reliably detect a physically missing display.
  Wire.beginTransmission(config::OLED_ADDRESS);
  bool ack = Wire.endTransmission() == 0;
  displayReady = ack && display.begin(SSD1306_SWITCHCAPVCC,
                                      config::OLED_ADDRESS, true, false);
  if (!displayReady) {
    Serial.println("OLED not found/allocation failed: check 3V3, GND, SDA21, SCL22 and 0x3C/0x3D.");
  } else {
    display.setTextWrap(false);
    display.dim(true);
  }
  loadState();
  lastMinute = lastSave = millis();
  Serial.println("Buddy ready. LEFT (32): previous. SELECT (33): choose/back. RIGHT (23): next.");
}

void loop() {
  uint32_t now = millis();
  input(now);
  while (uint32_t(now - lastMinute) >= 60000) {
    buddy.minute();
    dirty = true;
    lastMinute += 60000;
  }
  saveState(now);
  if (uint32_t(now - lastFrame) >= 50) {
    lastFrame = now;
    render(now);
    rgb(now);
  }
  delay(5);
}

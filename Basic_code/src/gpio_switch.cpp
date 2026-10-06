#include "gpio_switch.h"

// Reads the configured switch inputs and manages their pull-up mode.

namespace gpio_switch {
namespace {

bool pullupEnabled = true;

void applyMode() {
  for (uint8_t i = 0; i < kSwitchCount; ++i) {
    pinMode(device_config::kSwitchPins[i], pullupEnabled ? INPUT_PULLUP : INPUT);
  }
}

}  // namespace

void begin() {
  applyMode();
}

void setPullupEnabled(bool enabled) {
  pullupEnabled = enabled;
  applyMode();
}

bool isPullupEnabled() {
  return pullupEnabled;
}

bool isPressed() {
  return isPressed(0);
}

bool isPressed(uint8_t index) {
  if (index >= kSwitchCount) {
    return false;
  }
  return digitalRead(device_config::kSwitchPins[index]) == LOW;
}

int readLevel() {
  return readLevel(0);
}

int readLevel(uint8_t index) {
  if (index >= kSwitchCount) {
    return LOW;
  }
  return digitalRead(device_config::kSwitchPins[index]);
}

}  // namespace gpio_switch
#pragma once

#include <Arduino.h>

#include "device_config.h"

// Reads the configured physical switch inputs and manages pull-up mode.
namespace gpio_switch {

constexpr uint8_t kSwitchCount = device_config::kSwitchCount;

// Initializes all configured switch pins.
void begin();

// Enables or disables internal pull-up resistors on every switch pin.
void setPullupEnabled(bool enabled);

// Reports whether switches currently use input pull-ups.
bool isPullupEnabled();

// Returns whether the first switch is pressed.
bool isPressed();

// Returns whether a specific switch is pressed.
bool isPressed(uint8_t index);

// Returns the raw logic level of the first switch.
int readLevel();

// Returns the raw logic level of a specific switch.
int readLevel(uint8_t index);

}  // namespace gpio_switch
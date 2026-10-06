#pragma once

#include <Arduino.h>

#include "device_config.h"

// Owns the runtime LED state and drives the configured output pins.
namespace status_led {

constexpr uint8_t kLedCount = device_config::kLedCount;

// Initializes LED pins and restores persisted state when available.
bool begin();

// Sets the first LED channel on or off.
void setOn(bool on);

// Sets a specific LED channel on or off.
void setOn(uint8_t index, bool on);

// Toggles the first LED channel.
void toggle();

// Returns the state of the first LED channel.
bool isOn();

// Returns the state of a specific LED channel.
bool isOn(uint8_t index);

// Enables or disables persistence of LED state to EEPROM.
void setPersistenceEnabled(bool enabled);

// Reports whether LED state persistence is enabled.
bool isPersistenceEnabled();

}  // namespace status_led
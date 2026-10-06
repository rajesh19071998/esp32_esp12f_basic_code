#pragma once

#include <Arduino.h>

// Persists LED state and persistence settings in EEPROM.
namespace led_state_store {

// Initializes EEPROM access for LED persistence.
bool begin();

// Loads the first LED state from EEPROM.
bool load();

// Loads one LED state from EEPROM.
bool load(uint8_t index);

// Saves the first LED state to EEPROM.
bool save(bool ledOn);

// Saves one LED state to EEPROM.
bool save(uint8_t index, bool ledOn);

// Loads all LED states into the provided buffer.
bool loadAll(bool values[], size_t count);

// Saves all LED states from the provided buffer.
bool saveAll(const bool values[], size_t count);

// Reads whether persistence is enabled.
bool loadPersistenceEnabled();

// Saves whether persistence is enabled.
bool savePersistenceEnabled(bool enabled);

}  // namespace led_state_store
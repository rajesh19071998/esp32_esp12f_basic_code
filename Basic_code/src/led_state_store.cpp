#include "led_state_store.h"

#include <EEPROM.h>

// Stores LED states and persistence settings in a compact EEPROM layout.

namespace led_state_store {
namespace {

constexpr size_t kLedStateCount = 4;
constexpr int kLedStateAddress = 0;
constexpr int kPersistenceEnabledAddress = kLedStateAddress + kLedStateCount;
constexpr size_t kEepromSize = kLedStateCount + 1;
bool initialized = false;

bool isValidIndex(uint8_t index) {
  return index < kLedStateCount;
}

}  // namespace

bool begin() {
  if (initialized) {
    return true;
  }

#if defined(ESP32)
  initialized = EEPROM.begin(kEepromSize);
#elif defined(ESP8266)
  EEPROM.begin(kEepromSize);
  initialized = true;
#endif
  return initialized;
}

bool load() {
  return load(0);
}

bool load(uint8_t index) {
  if (!begin() || !isValidIndex(index)) {
    return false;
  }

  return EEPROM.read(kLedStateAddress + index) != 0;
}

bool save(bool ledOn) {
  return save(0, ledOn);
}

bool save(uint8_t index, bool ledOn) {
  if (!begin() || !isValidIndex(index)) {
    return false;
  }

  EEPROM.write(kLedStateAddress + index, ledOn ? 1 : 0);
  return EEPROM.commit();
}

bool loadAll(bool values[], size_t count) {
  if (!begin() || values == nullptr) {
    return false;
  }

  const size_t actualCount = (count > kEepromSize) ? kEepromSize : count;
  for (size_t i = 0; i < actualCount; ++i) {
    values[i] = load(static_cast<uint8_t>(i));
  }
  return true;
}

bool saveAll(const bool values[], size_t count) {
  if (!begin() || values == nullptr) {
    return false;
  }

  const size_t actualCount = (count > kLedStateCount) ? kLedStateCount : count;
  for (size_t i = 0; i < actualCount; ++i) {
    EEPROM.write(kLedStateAddress + i, values[i] ? 1 : 0);
  }
  return EEPROM.commit();
}

bool loadPersistenceEnabled() {
  if (!begin()) {
    return true;
  }

  return EEPROM.read(kPersistenceEnabledAddress) != 0;
}

bool savePersistenceEnabled(bool enabled) {
  if (!begin()) {
    return false;
  }

  EEPROM.write(kPersistenceEnabledAddress, enabled ? 1 : 0);
  return EEPROM.commit();
}

}  // namespace led_state_store
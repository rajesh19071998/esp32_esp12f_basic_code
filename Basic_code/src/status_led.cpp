#include "status_led.h"

#include "led_state_store.h"

// Owns runtime LED state, GPIO output writes, and persistence coordination.

namespace status_led {
namespace {

bool ledOn[kLedCount] = {};
bool persistenceEnabled = true;

void applyOutput(uint8_t index) {
  if (index >= kLedCount) {
    return;
  }
  digitalWrite(device_config::kLedPins[index], ledOn[index] ? HIGH : LOW);
}

}  // namespace

bool begin() {
  for (uint8_t i = 0; i < kLedCount; ++i) {
    pinMode(device_config::kLedPins[i], OUTPUT);
  }

  if (!led_state_store::begin()) {
    for (uint8_t i = 0; i < kLedCount; ++i) {
      applyOutput(i);
    }
    return false;
  }

  persistenceEnabled = led_state_store::loadPersistenceEnabled();

  if (!led_state_store::loadAll(ledOn, kLedCount)) {
    for (uint8_t i = 0; i < kLedCount; ++i) {
      ledOn[i] = false;
    }
  }

  for (uint8_t i = 0; i < kLedCount; ++i) {
    applyOutput(i);
  }
  return true;
}

void setOn(bool on) {
  setOn(0, on);
}

void setOn(uint8_t index, bool on) {
  if (index >= kLedCount) {
    return;
  }

  if (ledOn[index] == on) {
    return;
  }

  ledOn[index] = on;
  applyOutput(index);

  if (persistenceEnabled) {
    led_state_store::save(index, ledOn[index]);
  }
}

void toggle() {
  setOn(0, !ledOn[0]);
}

bool isOn() {
  return isOn(0);
}

bool isOn(uint8_t index) {
  if (index >= kLedCount) {
    return false;
  }
  return ledOn[index];
}

void setPersistenceEnabled(bool enabled) {
  persistenceEnabled = enabled;
  led_state_store::savePersistenceEnabled(persistenceEnabled);
  if (persistenceEnabled) {
    led_state_store::saveAll(ledOn, kLedCount);
  }
}

bool isPersistenceEnabled() {
  return persistenceEnabled;
}

}  // namespace status_led
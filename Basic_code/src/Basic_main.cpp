#include <Arduino.h>
#include "alexa_manager.h"
#include "led_state_store.h"
#include "device_config.h"
#include "esp_now_manager.h"
#include "gpio_switch.h"
#include "google_sheet.h"
#include "mdns_manager.h"
#include "mqtt_manager.h"
#include "ota.h"
#include "status_led.h"
#include "wifi_manager.h"

namespace Basic_main {

namespace {

bool previousSwitchPressed[gpio_switch::kSwitchCount] = {};

}  // namespace

// Initializes GPIO, restores persisted state, and starts enabled services.
void begin() {
  Serial.begin(115200);
  delay(250);

  gpio_switch::begin();
  const bool ledStateRestored = status_led::begin();

  for (uint8_t i = 0; i < gpio_switch::kSwitchCount; ++i) {
    previousSwitchPressed[i] = gpio_switch::isPressed(i);
  }

  Serial.printf("LED state restored from EEPROM: %s\n",
                ledStateRestored ? (status_led::isOn() ? "on" : "off") : "unavailable");

  mdns_manager::setupHostname(device_config::kDefaultHostname);
  mdns_manager::begin();
  ota::begin();
  esp_now_manager::begin();
  mqtt_manager::begin();
  google_sheet::sendDeviceInfo();
}

// Services enabled transports and mirrors physical switch changes into LED state.
void loop() {
  ota::handle();
  alexa_manager::handle();
  esp_now_manager::handle();
  mqtt_manager::handle();

  for (uint8_t i = 0; i < gpio_switch::kSwitchCount; ++i) {
    const bool switchPressed = gpio_switch::isPressed(i);
    if (switchPressed != previousSwitchPressed[i]) {
      previousSwitchPressed[i] = switchPressed;
      status_led::setOn(i, switchPressed);
      Serial.printf("Channel %s: switch on GPIO %d is %s, LED on GPIO %d is %s\n",
                    device_config::channelLabel(i),
                    device_config::kSwitchPins[i],
                    switchPressed ? "pressed" : "released",
                    device_config::kLedPins[i],
                    status_led::isOn(i) ? "on" : "off");
    }
  }
}

}  // namespace Basic_main

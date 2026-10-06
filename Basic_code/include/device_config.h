#pragma once

#include <Arduino.h>

// Central compile-time configuration for pins, labels, credentials, and defaults.
namespace device_config {

// I/O channel mapping used by src/gpio_switch.cpp, src/status_led.cpp, and src/main.cpp.
#if defined(ESP8266)
constexpr uint8_t kSwitchPins[] = {5 /* D1 */};
constexpr uint8_t kLedPins[] = {4 /* D2 */};
#else
constexpr uint8_t kSwitchPins[] = {21 /*, 23, 25 , 26 */ };
constexpr uint8_t kLedPins[] = {20 /*, 4, 18  , 19  */};
#endif

// LED display labels used by src/main.cpp, src/mqtt_manager.cpp, and src/web_led_control.cpp.
constexpr const char* const kChannelLabels[] = {"LED 1" /*, "LED 2", "LED 3" , "LED 4"*/ };

// Switch display labels used by src/mqtt_manager.cpp and src/web_led_control.cpp.
constexpr const char* const kSwitchLabels[] = {"Switch 1" /*, "Switch 2", "Switch 3" , "Switch 4"  */};

// Wi-Fi and access point defaults used by src/wifi_manager.cpp.
constexpr char kDefaultWifiSsid[] = "RajeshJioFiber";
constexpr char kDefaultWifiPassword[] = "Rajesh@1998";
#if defined(ESP8266)
constexpr char kDefaultAccessPointName[] = "ESP8266-OTA";
#else
constexpr char kDefaultAccessPointName[] = "ESP32-OTA";
#endif
constexpr char kDefaultAccessPointPassword[] = "firmware123";

// Device identity used by src/mdns_manager.cpp, src/main.cpp, and src/wifi_manager.cpp.
#if defined(ESP8266)
constexpr char kDefaultHostname[] = "esp8266-ota";
#else
constexpr char kDefaultHostname[] = "esp32-ota";
#endif

// Cloud reporting defaults used by src/google_sheet.cpp.
constexpr char kDefaultSheetUrl[] = "https://script.google.com/macros/s/AKfycbyx7WV7Yw3n1vM0WByS3IixRhTgS6SrcnP3LcMby0EJb3e6G3U/exec";
constexpr char kSoftwareVersion[] = "1.0.0";

// MQTT defaults used by src/mqtt_manager.cpp.
constexpr char kDefaultMqttBroker[] = "broker.hivemq.com";
constexpr uint16_t kDefaultMqttPort = 1883;
constexpr char kDefaultMqttUsername[] = "";
constexpr char kDefaultMqttPassword[] = "";
constexpr char kDefaultMqttPrefix[] = "esp32_ota";

// OTA login defaults used by src/ota.cpp.
constexpr char kDefaultOtaUsername[] = "admin";
constexpr char kDefaultOtaPassword[] = "admin";

// Wi-Fi persistence layout and timeout values used by src/wifi_manager.cpp.
constexpr uint16_t kWifiConfigEepromAddress = 32;
constexpr uint8_t kWifiConfigVersion = 1;
constexpr unsigned long kWifiConnectTimeoutMs = 15000;

// Derived counts used by include/gpio_switch.h, include/status_led.h, and include/web_dashboard_ui.h.
constexpr uint8_t kSwitchCount = sizeof(kSwitchPins) / sizeof(kSwitchPins[0]);
constexpr uint8_t kLedCount = sizeof(kLedPins) / sizeof(kLedPins[0]);
constexpr uint8_t kChannelCount = sizeof(kChannelLabels) / sizeof(kChannelLabels[0]);
constexpr uint8_t kSwitchLabelCount = sizeof(kSwitchLabels) / sizeof(kSwitchLabels[0]);

// Compile-time validation for arrays consumed across src/gpio_switch.cpp, src/status_led.cpp, and src/web_led_control.cpp.
static_assert(kSwitchCount == kLedCount, "Switch and LED counts must match");
static_assert(kChannelCount == kLedCount, "Channel labels must match LED and switch counts");
static_assert(kSwitchLabelCount == kSwitchCount, "Switch labels must match switch count");

inline const char* channelLabel(uint8_t index) {
  return index < kChannelCount ? kChannelLabels[index] : "Unknown";
}

// Returns a safe switch label for UI and telemetry callers.
inline const char* switchLabel(uint8_t index) {
  return index < kSwitchLabelCount ? kSwitchLabels[index] : "Unknown";
}

}  // namespace device_config
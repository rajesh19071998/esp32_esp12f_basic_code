#include "alexa_manager.h"

#ifndef ENABLE_ALEXA
#define ENABLE_ALEXA 0
#endif

#if ENABLE_ALEXA && (defined(ESP32) || defined(ESP8266))

#include <Espalexa.h>

#if defined(ESP32)
#include <WiFi.h>
#else
#include <ESP8266WiFi.h>
#endif

#include "device_config.h"
#include "status_led.h"

// Exposes each configured LED as an Alexa-compatible on/off device.

namespace alexa_manager {
namespace {

static_assert(status_led::kLedCount <= ESPALEXA_MAXDEVICES,
              "Increase ESPALEXA_MAXDEVICES for all configured LEDs");

Espalexa alexa;
EspalexaDevice* devices[status_led::kLedCount] = {};
bool ready = false;

void setLedFromAlexa(uint8_t index, EspalexaDevice* device) {
  if (index >= status_led::kLedCount || device == nullptr) {
    return;
  }

  status_led::setOn(index, device->getState());
  Serial.printf("Alexa set %s %s\n", device_config::channelLabel(index),
                status_led::isOn(index) ? "on" : "off");
}

}  // namespace

bool begin(WebServer& server) {
  if (WiFi.status() != WL_CONNECTED) {
    Serial.println("Alexa disabled: a Wi-Fi station connection is required");
    return false;
  }

  for (uint8_t i = 0; i < status_led::kLedCount; ++i) {
    devices[i] = new EspalexaDevice(
        device_config::channelLabel(i),
        [i](EspalexaDevice* device) { setLedFromAlexa(i, device); },
        EspalexaDeviceType::onoff,
        status_led::isOn(i) ? 255 : 0);

    if (alexa.addDevice(devices[i]) == 0) {
      Serial.printf("Alexa device registration failed for LED %u\n", i);
      return false;
    }
  }

  ready = alexa.begin(&server);
  Serial.printf("Alexa discovery %s with %u device(s)\n",
                ready ? "ready" : "failed", status_led::kLedCount);
  return ready;
}

void handle() {
  if (!ready) {
    return;
  }

  for (uint8_t i = 0; i < status_led::kLedCount; ++i) {
    if (devices[i] != nullptr && devices[i]->getState() != status_led::isOn(i)) {
      devices[i]->setState(status_led::isOn(i));
    }
  }

  alexa.loop();
}

bool handleApiCall(const String& uri, const String& body) {
  return ready && alexa.handleAlexaApiCall(uri, body);
}

bool isReady() {
  return ready;
}

}  // namespace alexa_manager

#else

namespace alexa_manager {

bool begin(WebServer&) { return false; }
void handle() {}
bool handleApiCall(const String&, const String&) { return false; }
bool isReady() { return false; }

}  // namespace alexa_manager

#endif
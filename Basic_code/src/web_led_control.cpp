#include "web_led_control.h"

#ifndef ENABLE_WEB_UI
#define ENABLE_WEB_UI 0
#endif

#if ENABLE_WEB_UI

#include <Arduino.h>

#include <Arduino.h>

#include "device_config.h"
#include "gpio_switch.h"
#include "google_sheet.h"
#include "status_led.h"
#include "web_dashboard_ui.h"
#include "wifi_manager.h"

// Implements the REST endpoints backing the dashboard and Wi-Fi settings pages.

namespace web_led_control {
namespace {

WebServer* gServer = nullptr;

String ledLabelsToJson() {
  String json = "[";
  for (uint8_t i = 0; i < device_config::kChannelCount; ++i) {
    if (i > 0) {
      json += ",";
    }
    json += '"';
    json += device_config::channelLabel(i);
    json += '"';
  }
  json += "]";
  return json;
}

String switchLabelsToJson() {
  String json = "[";
  for (uint8_t i = 0; i < device_config::kSwitchLabelCount; ++i) {
    if (i > 0) {
      json += ",";
    }
    json += '"';
    json += device_config::switchLabel(i);
    json += '"';
  }
  json += "]";
  return json;
}

String ledStateToJson() {
  String json = "{\"channelLabels\":" + ledLabelsToJson() + ",\"ledLabels\":" + ledLabelsToJson() + ",\"switchLabels\":" + switchLabelsToJson() + ",\"leds\":[";
  for (uint8_t i = 0; i < status_led::kLedCount; ++i) {
    if (i > 0) {
      json += ",";
    }
    json += status_led::isOn(i) ? "true" : "false";
  }
  json += "]}";
  return json;
}

String switchStateToJson() {
  String json = "{\"channelLabels\":" + ledLabelsToJson() + ",\"ledLabels\":" + ledLabelsToJson() + ",\"switchLabels\":" + switchLabelsToJson() + ",\"switches\":[";
  for (uint8_t i = 0; i < gpio_switch::kSwitchCount; ++i) {
    if (i > 0) {
      json += ",";
    }
    json += gpio_switch::isPressed(i) ? "true" : "false";
  }
  json += "]}";
  return json;
}

String persistenceStateToJson() {
  return String("{\"persistenceEnabled\":") + (status_led::isPersistenceEnabled() ? "true" : "false") + "}";
}

void handleGetLedState() {
  if (gServer == nullptr) {
    return;
  }
  gServer->send(200, "application/json", ledStateToJson());
}

void handleSetLedState() {
  if (gServer == nullptr) {
    return;
  }

  if (!gServer->hasArg("plain")) {
    gServer->send(400, "text/plain", "Missing JSON body");
    return;
  }

  const String body = gServer->arg("plain");
  const uint8_t ledCount = status_led::kLedCount;

  int firstIndex = -1;
  int secondIndex = -1;
  int stateValue = -1;

  if (sscanf(body.c_str(), "{\"led\":%d,\"state\":%d}", &firstIndex, &stateValue) == 2) {
    if (firstIndex >= 0 && firstIndex < ledCount) {
      status_led::setOn(firstIndex, stateValue != 0);
      gServer->send(200, "application/json", ledStateToJson());
      return;
    }
  }

  if (sscanf(body.c_str(), "{\"leds\":[%d,%d],\"states\":[%d,%d]}", &firstIndex, &secondIndex, &stateValue, &stateValue) == 4) {
    if (firstIndex >= 0 && firstIndex < ledCount && secondIndex >= 0 && secondIndex < ledCount) {
      status_led::setOn(firstIndex, stateValue != 0);
      status_led::setOn(secondIndex, stateValue != 0);
      gServer->send(200, "application/json", ledStateToJson());
      return;
    }
  }

  gServer->send(400, "text/plain", "Invalid payload");
}

void handleSetSingleLed() {
  if (gServer == nullptr) {
    return;
  }

  if (gServer->hasArg("led") && gServer->hasArg("state")) {
    const uint8_t ledIndex = gServer->arg("led").toInt();
    const bool isOn = gServer->arg("state").toInt() != 0;

    if (ledIndex < status_led::kLedCount) {
      status_led::setOn(ledIndex, isOn);
      gServer->send(200, "application/json", ledStateToJson());
      return;
    }
  }

  gServer->send(400, "text/plain", "Missing or invalid led/state parameter");
}

void handleGetSwitchState() {
  if (gServer == nullptr) {
    return;
  }
  gServer->send(200, "application/json", switchStateToJson());
}

void handleGetDeviceInfo() {
  if (gServer == nullptr) {
    return;
  }
  gServer->send(200, "application/json", google_sheet::getDeviceInfoJson());
}

void handlePersistState() {
  if (gServer == nullptr) {
    return;
  }

  if (gServer->hasArg("enabled")) {
    const bool enabled = gServer->arg("enabled").toInt() != 0;
    status_led::setPersistenceEnabled(enabled);
    gServer->send(200, "application/json", persistenceStateToJson());
    return;
  }

  gServer->send(200, "application/json", persistenceStateToJson());
}

String wifiSettingsJson() {
  return String("{\"wifiSsid\":\"") + wifi_manager::getWifiSsid() +
         "\",\"wifiPassword\":\"" + wifi_manager::getWifiPassword() +
         "\",\"apName\":\"" + wifi_manager::getAccessPointName() +
         "\",\"apPassword\":\"" + wifi_manager::getAccessPointPassword() + "\"}";
}

void handleWifiSettings() {
  if (gServer == nullptr) {
    return;
  }

  if (gServer->method() == HTTP_POST) {
    String ssid = gServer->hasArg("wifiSsid") ? gServer->arg("wifiSsid") : "";
    String password = gServer->hasArg("wifiPassword") ? gServer->arg("wifiPassword") : "";
    String apName = gServer->hasArg("apName") ? gServer->arg("apName") : "";
    String apPassword = gServer->hasArg("apPassword") ? gServer->arg("apPassword") : "";

    ssid.trim();
    password.trim();
    apName.trim();
    apPassword.trim();

    wifi_manager::setWifiConfig(
      ssid.isEmpty() ? nullptr : ssid.c_str(),
      password.isEmpty() ? nullptr : password.c_str(),
      apName.isEmpty() ? nullptr : apName.c_str(),
      apPassword.isEmpty() ? nullptr : apPassword.c_str());

    gServer->send(200, "application/json", String("{\"status\":\"saved\"}"));
    return;
  }

  if (gServer->hasArg("json") && gServer->arg("json") == "1") {
    gServer->send(200, "application/json", wifiSettingsJson());
    return;
  }

  gServer->send(200, "text/html", WIFI_SETTINGS_UI);
}

}  // namespace

void registerRoutes(WebServer& server) {
  gServer = &server;
  server.on("/leds", HTTP_GET, handleGetLedState);
  server.on("/leds", HTTP_POST, handleSetLedState);
  server.on("/led", HTTP_GET, handleGetLedState);
  server.on("/led", HTTP_POST, handleSetSingleLed);
  server.on("/switches", HTTP_GET, handleGetSwitchState);
  server.on("/device", HTTP_GET, handleGetDeviceInfo);
  server.on("/persist", HTTP_GET, handlePersistState);
  server.on("/persist", HTTP_POST, handlePersistState);
  server.on("/settings", HTTP_GET, handlePersistState);
  server.on("/settings", HTTP_POST, handlePersistState);
  server.on("/wifi-settings", HTTP_GET, handleWifiSettings);
  server.on("/wifi-settings", HTTP_POST, handleWifiSettings);
}

}  // namespace web_led_control

#else

namespace web_led_control {

void registerRoutes(WebServer&) {}

}  // namespace web_led_control

#endif

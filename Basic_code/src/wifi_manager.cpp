#include "wifi_manager.h"

#include <Arduino.h>
#include <EEPROM.h>
#if defined(ESP32)
#include <WiFi.h>
#elif defined(ESP8266)
#include <ESP8266WiFi.h>
#endif

#include "device_config.h"
#include "google_sheet.h"
#include "mdns_manager.h"

// Persists Wi-Fi credentials, manages station/AP mode, and reports network state.

namespace wifi_manager {
namespace {

char wifiSsid[32] = "";
char wifiPassword[32] = "";
char accessPointName[32] = "";
char accessPointPassword[32] = "";

void applyDefaults() {
  strncpy(wifiSsid, device_config::kDefaultWifiSsid, sizeof(wifiSsid) - 1);
  strncpy(wifiPassword, device_config::kDefaultWifiPassword, sizeof(wifiPassword) - 1);
  strncpy(accessPointName, device_config::kDefaultAccessPointName, sizeof(accessPointName) - 1);
  strncpy(accessPointPassword, device_config::kDefaultAccessPointPassword, sizeof(accessPointPassword) - 1);
  wifiSsid[sizeof(wifiSsid) - 1] = '\0';
  wifiPassword[sizeof(wifiPassword) - 1] = '\0';
  accessPointName[sizeof(accessPointName) - 1] = '\0';
  accessPointPassword[sizeof(accessPointPassword) - 1] = '\0';
}

void writeStringToEeprom(uint16_t offset, const char* value, size_t maxLen) {
  for (size_t i = 0; i < maxLen; ++i) {
    EEPROM.write(offset + i, (value != nullptr && value[i] != '\0') ? value[i] : '\0');
  }
}

String readStringFromEeprom(uint16_t offset, size_t maxLen) {
  String value;
  for (size_t i = 0; i < maxLen; ++i) {
    const char c = static_cast<char>(EEPROM.read(offset + i));
    if (c == '\0') {
      break;
    }
    value += c;
  }
  return value;
}

void loadFromEeprom() {
  EEPROM.begin(256);
  const uint8_t version = EEPROM.read(device_config::kWifiConfigEepromAddress);
  if (version != device_config::kWifiConfigVersion) {
    applyDefaults();
    EEPROM.end();
    return;
  }

  const String storedWifiSsid = readStringFromEeprom(device_config::kWifiConfigEepromAddress + 1, sizeof(wifiSsid));
  const String storedWifiPassword = readStringFromEeprom(device_config::kWifiConfigEepromAddress + 1 + sizeof(wifiSsid), sizeof(wifiPassword));
  const String storedApName = readStringFromEeprom(device_config::kWifiConfigEepromAddress + 1 + sizeof(wifiSsid) + sizeof(wifiPassword), sizeof(accessPointName));
  const String storedApPassword = readStringFromEeprom(device_config::kWifiConfigEepromAddress + 1 + sizeof(wifiSsid) + sizeof(wifiPassword) + sizeof(accessPointName), sizeof(accessPointPassword));

  if (!storedWifiSsid.isEmpty()) {
    strncpy(wifiSsid, storedWifiSsid.c_str(), sizeof(wifiSsid) - 1);
    wifiSsid[sizeof(wifiSsid) - 1] = '\0';
  } else {
    applyDefaults();
  }

  if (!storedWifiPassword.isEmpty()) {
    strncpy(wifiPassword, storedWifiPassword.c_str(), sizeof(wifiPassword) - 1);
    wifiPassword[sizeof(wifiPassword) - 1] = '\0';
  } else {
    applyDefaults();
  }

  if (!storedApName.isEmpty()) {
    strncpy(accessPointName, storedApName.c_str(), sizeof(accessPointName) - 1);
    accessPointName[sizeof(accessPointName) - 1] = '\0';
  } else {
    applyDefaults();
  }

  if (!storedApPassword.isEmpty()) {
    strncpy(accessPointPassword, storedApPassword.c_str(), sizeof(accessPointPassword) - 1);
    accessPointPassword[sizeof(accessPointPassword) - 1] = '\0';
  } else {
    applyDefaults();
  }

  EEPROM.end();
}

void saveToEeprom() {
  EEPROM.begin(256);
  EEPROM.write(device_config::kWifiConfigEepromAddress, device_config::kWifiConfigVersion);
  writeStringToEeprom(device_config::kWifiConfigEepromAddress + 1, wifiSsid, sizeof(wifiSsid));
  writeStringToEeprom(device_config::kWifiConfigEepromAddress + 1 + sizeof(wifiSsid), wifiPassword, sizeof(wifiPassword));
  writeStringToEeprom(device_config::kWifiConfigEepromAddress + 1 + sizeof(wifiSsid) + sizeof(wifiPassword), accessPointName, sizeof(accessPointName));
  writeStringToEeprom(device_config::kWifiConfigEepromAddress + 1 + sizeof(wifiSsid) + sizeof(wifiPassword) + sizeof(accessPointName), accessPointPassword, sizeof(accessPointPassword));
  EEPROM.commit();
  EEPROM.end();
}

}  // namespace

const char* getWifiSsid() {
  return wifiSsid;
}

const char* getWifiPassword() {
  return wifiPassword;
}

const char* getAccessPointName() {
  return accessPointName;
}

const char* getAccessPointPassword() {
  return accessPointPassword;
}

void setWifiConfig(const char* ssid, const char* password, const char* apName, const char* apPassword) {
  if (ssid != nullptr && ssid[0] != '\0') {
    strncpy(wifiSsid, ssid, sizeof(wifiSsid) - 1);
    wifiSsid[sizeof(wifiSsid) - 1] = '\0';
  }

  if (password != nullptr && password[0] != '\0') {
    strncpy(wifiPassword, password, sizeof(wifiPassword) - 1);
    wifiPassword[sizeof(wifiPassword) - 1] = '\0';
  }

  if (apName != nullptr && apName[0] != '\0') {
    strncpy(accessPointName, apName, sizeof(accessPointName) - 1);
    accessPointName[sizeof(accessPointName) - 1] = '\0';
  }

  if (apPassword != nullptr && apPassword[0] != '\0') {
    strncpy(accessPointPassword, apPassword, sizeof(accessPointPassword) - 1);
    accessPointPassword[sizeof(accessPointPassword) - 1] = '\0';
  }

  saveToEeprom();
}

void startAccessPoint() {
  WiFi.mode(WIFI_AP);
  WiFi.softAP(accessPointName, accessPointPassword);

  const IPAddress ip = WiFi.softAPIP();
  Serial.println();
  Serial.printf("OTA AP ready: %s\n", accessPointName);
  Serial.printf("Password: %s\n", accessPointPassword);
  Serial.printf("Open http://%s in your browser\n", ip.toString().c_str());
}

void reconnect() {
  WiFi.disconnect(true);
  WiFi.mode(WIFI_STA);
  WiFi.begin(wifiSsid, wifiPassword);

  const unsigned long startMs = millis();
  while (WiFi.status() != WL_CONNECTED && millis() - startMs < device_config::kWifiConnectTimeoutMs) {
    delay(200);
  }

  if (WiFi.status() == WL_CONNECTED) {
    Serial.println();
    Serial.print("Connected to WiFi: ");
    Serial.println(wifiSsid);
    Serial.print("IP address: ");
    Serial.println(WiFi.localIP());
    return;
  }

  Serial.println();
  Serial.printf("WiFi connection failed. Falling back to AP: %s\n", accessPointName);
  startAccessPoint();
}

void begin() {
  applyDefaults();
  loadFromEeprom();
  mdns_manager::setupHostname(device_config::kDefaultHostname);
  reconnect();
  google_sheet::sendDeviceInfo();
}

}  // namespace wifi_manager

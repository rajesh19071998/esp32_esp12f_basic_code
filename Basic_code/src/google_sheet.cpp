#include "google_sheet.h"

#include <Arduino.h>
#if defined(ESP32)
#include <HTTPClient.h>
#include <WiFi.h>
#include <WiFiClientSecure.h>
#elif defined(ESP8266)
#include <ESP8266HTTPClient.h>
#include <ESP8266WiFi.h>
#include <WiFiClientSecure.h>
#endif

#include "device_config.h"
#include "wifi_manager.h"

// Builds device-status payloads and reports them to a Google Apps Script endpoint.

namespace google_sheet {
namespace {

const char* endpointUrl = device_config::kDefaultSheetUrl;

String urlEncode(const String& value) {
  String encoded;
  for (size_t i = 0; i < value.length(); ++i) {
    const char c = value[i];
    if (isalnum((unsigned char)c) || c == '-' || c == '_' || c == '.' || c == '~') {
      encoded += c;
    } else if (c == ' ') {
      encoded += "+";
    } else {
      encoded += '%';
      if (static_cast<uint8_t>(c) < 0x10) {
        encoded += '0';
      }
      encoded += String(static_cast<uint8_t>(c), HEX);
    }
  }
  return encoded;
}

String deviceHostname() {
  return WiFi.getHostname();
}

String deviceIp() {
  return WiFi.localIP().toString();
}

String mdnsHost() {
  return deviceHostname() + ".local";
}

String wifiModeName() {
  const uint8_t mode = WiFi.getMode();
  if (mode == WIFI_AP) {
    return "Hotspot";
  }
  if (mode == WIFI_STA) {
    return "Wi‑Fi";
  }
  if (mode == WIFI_AP_STA) {
    return "Wi‑Fi + Hotspot";
  }
  return "Unknown";
}

String uptimeString() {
  uint32_t totalSeconds = millis() / 1000UL;
  const uint32_t hours = totalSeconds / 3600UL;
  totalSeconds %= 3600UL;
  const uint32_t minutes = totalSeconds / 60UL;
  const uint32_t seconds = totalSeconds % 60UL;

  char buffer[32];
  snprintf(buffer, sizeof(buffer), "%02lu:%02lu:%02lu", static_cast<unsigned long>(hours), static_cast<unsigned long>(minutes), static_cast<unsigned long>(seconds));
  return String(buffer);
}

String hotspotIpString() {
  if (WiFi.getMode() == WIFI_AP || WiFi.getMode() == WIFI_AP_STA) {
    return WiFi.softAPIP().toString();
  }
  return "";
}

}  // namespace

void setEndpoint(const char* url) {
  endpointUrl = (url != nullptr && url[0] != '\0') ? url : device_config::kDefaultSheetUrl;
}

const char* getSoftwareVersion() {
  return device_config::kSoftwareVersion;
}

const char* getBuildDate() {
  return __DATE__;
}

const char* getBuildTime() {
  return __TIME__;
}

String getMemoryInfoJson() {
#if defined(ESP32)
  const uint32_t totalRam = ESP.getHeapSize();
#elif defined(ESP8266)
  const uint32_t totalRam = 81920;
#endif
  const uint32_t freeRam = ESP.getFreeHeap();
  const uint32_t usedRam = totalRam - freeRam;

  const uint32_t totalFlash = ESP.getFlashChipSize();
  const uint32_t usedFlash = ESP.getSketchSize();
  const uint32_t freeFlash = ESP.getFreeSketchSpace();

  return String("{\"ramTotal\":") + totalRam +
         ",\"ramUsed\":" + usedRam +
         ",\"ramFree\":" + freeRam +
         ",\"flashTotal\":" + totalFlash +
         ",\"flashUsed\":" + usedFlash +
         ",\"flashFree\":" + freeFlash + "}";
}

String getDeviceInfoJson() {
#if defined(ESP32)
  const uint32_t totalRam = ESP.getHeapSize();
#elif defined(ESP8266)
  const uint32_t totalRam = 81920;
#endif
  const uint32_t freeRam = ESP.getFreeHeap();
  const uint32_t usedRam = totalRam - freeRam;

  const uint32_t totalFlash = ESP.getFlashChipSize();
  const uint32_t usedFlash = ESP.getSketchSize();

  return String("{\"hostname\":\"") + deviceHostname() +
         "\",\"ip\":\"" + deviceIp() +
         "\",\"mdns\":\"" + mdnsHost() +
      "\",\"swVersion\":\"" + device_config::kSoftwareVersion +
         "\",\"buildDate\":\"" + __DATE__ +
         "\",\"buildTime\":\"" + __TIME__ +
         "\",\"ramTotal\":" + totalRam +
         ",\"ramUsed\":" + usedRam +
         ",\"flashTotal\":" + totalFlash +
         ",\"flashUsed\":" + usedFlash +
         ",\"wifiMode\":\"" + wifiModeName() +
         "\",\"hotspotName\":\"" + wifi_manager::getAccessPointName() +
         "\",\"hotspotPassword\":\"" + wifi_manager::getAccessPointPassword() +
         "\",\"hotspotIp\":\"" + hotspotIpString() +
         "\",\"uptime\":\"" + uptimeString() + "\"}";
}

void sendDeviceInfo() {
  if (WiFi.status() != WL_CONNECTED) {
    Serial.println("WiFi is not connected, skipping Google Sheet update.");
    return;
  }

  const String hostname = deviceHostname();
  const String ipAddress = deviceIp();
  const String mdnsHostValue = mdnsHost();

  WiFiClientSecure client;
  client.setInsecure();

  HTTPClient http;
#if defined(ESP32)
  http.begin(client, endpointUrl);
#elif defined(ESP8266)
  http.begin(client, endpointUrl);
#endif
  http.addHeader("Content-Type", "application/x-www-form-urlencoded");

  const String payload =
      String("hostname=") + urlEncode(hostname) +
      "&ip=" + urlEncode(ipAddress) +
      "&mdns=" + urlEncode(mdnsHostValue) +
      "&swVersion=" + urlEncode(device_config::kSoftwareVersion) +
      "&buildDate=" + urlEncode(__DATE__) +
      "&buildTime=" + urlEncode(__TIME__) +
      "&timestamp=" + urlEncode(String(millis()));

  const int httpCode = http.POST(payload);

  if (httpCode > 0) {
    const String response = http.getString();
    Serial.printf("Google Sheet update HTTP code: %d\n", httpCode);
    Serial.println(response);
  } else {
    Serial.printf("Google Sheet update failed, error: %s\n", http.errorToString(httpCode).c_str());
  }

  http.end();
}

}  // namespace google_sheet

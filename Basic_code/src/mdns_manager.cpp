#include "mdns_manager.h"

#ifndef ENABLE_MDNS
#define ENABLE_MDNS 0
#endif

#if ENABLE_MDNS

#include <Arduino.h>
#if defined(ESP32)
#include <WiFi.h>
#include <ESPmDNS.h>
#elif defined(ESP8266)
#include <ESP8266WiFi.h>
#include <ESP8266mDNS.h>
#endif

#include "device_config.h"

// Applies the configured hostname and advertises the HTTP service over mDNS.

namespace mdns_manager {

void setupHostname(const char* hostname) {
  const char* resolvedHostname = (hostname != nullptr && hostname[0] != '\0')
                                    ? hostname
                                    : device_config::kDefaultHostname;

#if defined(ESP32)
  WiFi.setHostname(resolvedHostname);
#elif defined(ESP8266)
  WiFi.hostname(resolvedHostname);
#endif

  Serial.printf("WiFi hostname set to: %s\n", resolvedHostname);
}

void begin() {
#if defined(ESP32)
  if (!MDNS.begin(device_config::kDefaultHostname)) {
    Serial.println("Failed to start mDNS");
    return;
  }

  Serial.println("mDNS responder started");
  MDNS.addService("http", "tcp", 80);
#elif defined(ESP8266)
  if (!MDNS.begin(device_config::kDefaultHostname)) {
    Serial.println("Failed to start mDNS");
    return;
  }

  Serial.println("mDNS responder started");
  MDNS.addService("http", "tcp", 80);
#endif
}

}  // namespace mdns_manager

#else

namespace mdns_manager {

void setupHostname(const char*) {}
void begin() {}

}  // namespace mdns_manager

#endif

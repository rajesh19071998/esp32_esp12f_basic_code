#pragma once

#include <Arduino.h>

#if defined(ESP32)
#include <WebServer.h>
#elif defined(ESP8266)
#include <ESP8266WebServer.h>
using WebServer = ESP8266WebServer;
#endif

// Integrates Espalexa with the shared HTTP server and LED state model.
namespace alexa_manager {

// Registers Alexa devices against the shared web server.
bool begin(WebServer& server);

// Keeps Alexa-reported device state in sync with local LED state.
void handle();

// Lets the shared not-found handler pass Alexa Hue API requests through.
bool handleApiCall(const String& uri, const String& body);

// Reports whether Alexa discovery and request handling are active.
bool isReady();

}  // namespace alexa_manager
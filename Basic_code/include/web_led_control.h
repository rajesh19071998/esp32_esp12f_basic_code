#pragma once

#if defined(ESP32)
#include <WebServer.h>
#elif defined(ESP8266)
#include <ESP8266WebServer.h>
using WebServer = ESP8266WebServer;
#endif

// Registers REST-style dashboard routes for LED, switch, and Wi-Fi control.
namespace web_led_control {

// Adds all dashboard and JSON API routes to the shared HTTP server.
void registerRoutes(WebServer& server);

}  // namespace web_led_control

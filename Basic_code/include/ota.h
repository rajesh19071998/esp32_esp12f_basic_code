#pragma once

// Owns the shared HTTP server used for OTA, dashboard routes, and Alexa hooks.
namespace ota {

// Starts Wi-Fi and any enabled HTTP-based services.
void begin();

// Services HTTP requests and delayed restart requests.
void handle();

}  // namespace ota
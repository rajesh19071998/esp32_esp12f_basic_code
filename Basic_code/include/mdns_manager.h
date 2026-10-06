#pragma once

// Configures and starts the mDNS responder for hostname-based access.
namespace mdns_manager {

// Starts the mDNS responder and HTTP service advertisement.
void begin();

// Applies the board hostname to the active Wi-Fi stack.
void setupHostname(const char* hostname);

}  // namespace mdns_manager

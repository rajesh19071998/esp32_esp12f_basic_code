#pragma once

// Stores Wi-Fi credentials, connects as a station, and falls back to access-point mode.
namespace wifi_manager {

// Loads stored credentials and connects using the active configuration.
void begin();

// Reconnects the station interface or falls back to the configured access point.
void reconnect();

// Starts the configured access point immediately.
void startAccessPoint();

// Updates the stored station and access-point credentials.
void setWifiConfig(const char* ssid, const char* password, const char* apName, const char* apPassword);

// Returns the configured station SSID.
const char* getWifiSsid();

// Returns the configured station password.
const char* getWifiPassword();

// Returns the configured access-point name.
const char* getAccessPointName();

// Returns the configured access-point password.
const char* getAccessPointPassword();

}  // namespace wifi_manager

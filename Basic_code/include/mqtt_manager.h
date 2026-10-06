#pragma once

#include <Arduino.h>

// Publishes device state to MQTT and applies incoming LED control messages.
namespace mqtt_manager {

// Applies the configured broker, credentials, and topic prefix.
void begin();

// Maintains the broker connection and processes incoming MQTT messages.
void handle();

// Publishes the latest device status JSON to the MQTT state topic.
void publishStatus();

// Overrides the MQTT broker endpoint and optional credentials.
void setBroker(const char* host, uint16_t port, const char* username = nullptr, const char* password = nullptr);

// Overrides the MQTT topic prefix used for state and commands.
void setTopicPrefix(const char* prefix);

// Reports whether the MQTT client is currently connected.
bool isConnected();

}  // namespace mqtt_manager

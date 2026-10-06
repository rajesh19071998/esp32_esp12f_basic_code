#pragma once

#include <Arduino.h>

// Broadcasts board status over ESP-NOW and accepts remote LED control packets.
namespace esp_now_manager {

// Initializes ESP-NOW for the current board and Wi-Fi mode.
void begin();

// Applies queued control packets and publishes state changes.
void handle();

// Broadcasts the current LED, switch, and board status snapshot.
void publishStatus();

// Sends a remote LED control packet for the selected channel.
bool sendLedControl(uint8_t ledIndex, bool on);

// Reports whether ESP-NOW transport is active.
bool isReady();

}  // namespace esp_now_manager
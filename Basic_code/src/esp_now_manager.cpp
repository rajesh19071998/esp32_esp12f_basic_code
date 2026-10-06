#include "esp_now_manager.h"

#ifndef ENABLE_ESP_NOW
#define ENABLE_ESP_NOW 0
#endif

#if ENABLE_ESP_NOW && (defined(ESP32) || defined(ESP8266))

#if defined(ESP32)
#include <WiFi.h>
#include <esp_idf_version.h>
#include <esp_now.h>
#elif defined(ESP8266)
#include <ESP8266WiFi.h>
#include <espnow.h>
#endif

#include "device_config.h"
#include "gpio_switch.h"
#include "status_led.h"
#include "wifi_manager.h"

// Broadcasts local state over ESP-NOW and applies inbound LED control packets.

namespace esp_now_manager {
namespace {

constexpr uint32_t kPacketMagic = 0x45534E57;
constexpr uint8_t kProtocolVersion = 1;
constexpr unsigned long kPublishIntervalMs = 5000;
constexpr uint8_t kBroadcastAddress[] = {0xff, 0xff, 0xff, 0xff, 0xff, 0xff};

enum class MessageType : uint8_t {
  Status = 1,
  LedControl = 2,
};

struct __attribute__((packed)) Packet {
  uint32_t magic;
  uint8_t version;
  MessageType type;
  uint8_t ledCount;
  uint8_t switchCount;
  uint32_t ledMask;
  uint32_t switchMask;
  uint32_t controlMask;
  uint32_t controlValues;
  uint8_t wifiMode;
  uint8_t wifiChannel;
  uint8_t ip[4];
  char hostname[32];
  char ssid[32];
};

static_assert(sizeof(Packet) <= 250, "ESP-NOW packet is too large");
static_assert(status_led::kLedCount <= 32, "ESP-NOW supports up to 32 LEDs");
static_assert(gpio_switch::kSwitchCount <= 32, "ESP-NOW supports up to 32 switches");

bool ready = false;
bool publishRequested = false;
unsigned long lastPublishMs = 0;
uint32_t lastLedMask = 0;
uint32_t lastSwitchMask = 0;
volatile uint32_t pendingControlMask = 0;
volatile uint32_t pendingControlValues = 0;
#if defined(ESP32)
portMUX_TYPE controlMux = portMUX_INITIALIZER_UNLOCKED;
#endif

bool isAccessPointOnly() {
#if defined(ESP32)
  return WiFi.getMode() == WIFI_MODE_AP;
#else
  return WiFi.getMode() == WIFI_AP;
#endif
}

void lockControl() {
#if defined(ESP32)
  portENTER_CRITICAL(&controlMux);
#else
  noInterrupts();
#endif
}

void unlockControl() {
#if defined(ESP32)
  portEXIT_CRITICAL(&controlMux);
#else
  interrupts();
#endif
}

uint32_t readLedMask() {
  uint32_t mask = 0;
  for (uint8_t i = 0; i < status_led::kLedCount; ++i) {
    if (status_led::isOn(i)) {
      mask |= (1UL << i);
    }
  }
  return mask;
}

uint32_t readSwitchMask() {
  uint32_t mask = 0;
  for (uint8_t i = 0; i < gpio_switch::kSwitchCount; ++i) {
    if (gpio_switch::isPressed(i)) {
      mask |= (1UL << i);
    }
  }
  return mask;
}

Packet buildPacket(MessageType type) {
  Packet packet = {};
  packet.magic = kPacketMagic;
  packet.version = kProtocolVersion;
  packet.type = type;
  packet.ledCount = status_led::kLedCount;
  packet.switchCount = gpio_switch::kSwitchCount;
  packet.ledMask = readLedMask();
  packet.switchMask = readSwitchMask();
  packet.wifiMode = static_cast<uint8_t>(WiFi.getMode());
  packet.wifiChannel = WiFi.channel();

  const IPAddress ip = isAccessPointOnly() ? WiFi.softAPIP() : WiFi.localIP();
  for (uint8_t i = 0; i < 4; ++i) {
    packet.ip[i] = ip[i];
  }

#if defined(ESP32)
  const char* hostname = WiFi.getHostname();
#else
  const char* hostname = device_config::kDefaultHostname;
#endif
  strncpy(packet.hostname, hostname != nullptr ? hostname : device_config::kDefaultHostname,
          sizeof(packet.hostname) - 1);
  strncpy(packet.ssid, isAccessPointOnly() ? wifi_manager::getAccessPointName()
                                           : wifi_manager::getWifiSsid(),
          sizeof(packet.ssid) - 1);
  return packet;
}

bool sendPacket(const Packet& packet) {
#if defined(ESP32)
  return ready && esp_now_send(kBroadcastAddress,
                               reinterpret_cast<const uint8_t*>(&packet),
                               sizeof(packet)) == ESP_OK;
#else
  return ready && esp_now_send(const_cast<uint8_t*>(kBroadcastAddress),
                               reinterpret_cast<uint8_t*>(const_cast<Packet*>(&packet)),
                               sizeof(packet)) == 0;
#endif
}

void processReceivedData(const uint8_t* data, int length) {
  if (data == nullptr || length != sizeof(Packet)) {
    return;
  }

  Packet packet;
  memcpy(&packet, data, sizeof(packet));
  if (packet.magic != kPacketMagic || packet.version != kProtocolVersion) {
    return;
  }
  packet.hostname[sizeof(packet.hostname) - 1] = '\0';
  packet.ssid[sizeof(packet.ssid) - 1] = '\0';

  if (packet.type == MessageType::Status) {
    Serial.printf("ESP-NOW status: %s, IP %u.%u.%u.%u, LEDs 0x%08lx, switches 0x%08lx\n",
                  packet.hostname, packet.ip[0], packet.ip[1], packet.ip[2], packet.ip[3],
                  static_cast<unsigned long>(packet.ledMask),
                  static_cast<unsigned long>(packet.switchMask));
    return;
  }

  if (packet.type != MessageType::LedControl) {
    return;
  }

  lockControl();
  pendingControlValues = (pendingControlValues & ~packet.controlMask) |
                         (packet.controlValues & packet.controlMask);
  pendingControlMask |= packet.controlMask;
  unlockControl();
}

#if defined(ESP32) && ESP_IDF_VERSION_MAJOR >= 5
void onDataReceived(const esp_now_recv_info_t*, const uint8_t* data, int length) {
  processReceivedData(data, length);
}
#elif defined(ESP32)
void onDataReceived(const uint8_t*, const uint8_t* data, int length) {
  processReceivedData(data, length);
}
#else
void onDataReceived(uint8_t*, uint8_t* data, uint8_t length) {
  processReceivedData(data, length);
}
#endif

}  // namespace

void begin() {
#if defined(ESP32)
  if (esp_now_init() != ESP_OK) {
    Serial.println("ESP-NOW initialization failed");
    return;
  }

  esp_now_register_recv_cb(onDataReceived);

  esp_now_peer_info_t peer = {};
  memcpy(peer.peer_addr, kBroadcastAddress, sizeof(kBroadcastAddress));
  peer.channel = 0;
  peer.encrypt = false;
  peer.ifidx = isAccessPointOnly() ? WIFI_IF_AP : WIFI_IF_STA;

  if (esp_now_add_peer(&peer) != ESP_OK) {
#else
  if (esp_now_init() != 0) {
    Serial.println("ESP-NOW initialization failed");
    return;
  }

  esp_now_set_self_role(ESP_NOW_ROLE_COMBO);
  esp_now_register_recv_cb(onDataReceived);

  if (esp_now_add_peer(const_cast<uint8_t*>(kBroadcastAddress), ESP_NOW_ROLE_COMBO,
                       WiFi.channel(), nullptr, 0) != 0) {
#endif
    Serial.println("ESP-NOW broadcast peer setup failed");
    esp_now_deinit();
    return;
  }

  ready = true;
  publishRequested = true;
  Serial.printf("ESP-NOW ready on Wi-Fi channel %u\n", WiFi.channel());
}

void handle() {
  if (!ready) {
    return;
  }

  uint32_t controlMask;
  uint32_t controlValues;
  lockControl();
  controlMask = pendingControlMask;
  controlValues = pendingControlValues;
  pendingControlMask = 0;
  unlockControl();

  for (uint8_t i = 0; i < status_led::kLedCount; ++i) {
    const uint32_t bit = 1UL << i;
    if ((controlMask & bit) != 0) {
      status_led::setOn(i, (controlValues & bit) != 0);
      publishRequested = true;
    }
  }

  const uint32_t ledMask = readLedMask();
  const uint32_t switchMask = readSwitchMask();
  const unsigned long now = millis();
  if (publishRequested || ledMask != lastLedMask || switchMask != lastSwitchMask ||
      now - lastPublishMs >= kPublishIntervalMs) {
    publishStatus();
  }
}

void publishStatus() {
  if (!ready) {
    return;
  }

  const Packet packet = buildPacket(MessageType::Status);
  if (sendPacket(packet)) {
    lastLedMask = packet.ledMask;
    lastSwitchMask = packet.switchMask;
    lastPublishMs = millis();
    publishRequested = false;
  }
}

bool sendLedControl(uint8_t ledIndex, bool on) {
  if (!ready || ledIndex >= status_led::kLedCount) {
    return false;
  }

  Packet packet = buildPacket(MessageType::LedControl);
  packet.controlMask = 1UL << ledIndex;
  packet.controlValues = on ? packet.controlMask : 0;
  return sendPacket(packet);
}

bool isReady() {
  return ready;
}

}  // namespace esp_now_manager

#else

namespace esp_now_manager {

void begin() {}
void handle() {}
void publishStatus() {}
bool sendLedControl(uint8_t, bool) { return false; }
bool isReady() { return false; }

}  // namespace esp_now_manager

#endif
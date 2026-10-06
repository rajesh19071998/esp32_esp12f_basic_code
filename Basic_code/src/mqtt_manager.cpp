#include "mqtt_manager.h"

#ifndef ENABLE_MQTT
#define ENABLE_MQTT 0
#endif

#if ENABLE_MQTT

#include <Arduino.h>
#include <PubSubClient.h>

#if defined(ESP32)
#include <WiFi.h>
#elif defined(ESP8266)
#include <ESP8266WiFi.h>
#endif

#include "device_config.h"
#include "gpio_switch.h"
#include "google_sheet.h"
#include "status_led.h"
#include "wifi_manager.h"

// Publishes device status to MQTT and applies incoming LED control messages.

namespace mqtt_manager {
namespace {

char brokerHost[64];
uint16_t brokerPort = device_config::kDefaultMqttPort;
char mqttUsername[32];
char mqttPassword[32];
char topicPrefix[64];

WiFiClient wifiClient;
PubSubClient client(wifiClient);

String buildChannelLabelsJson() {
  String json = "[";
  for (uint8_t i = 0; i < device_config::kChannelCount; ++i) {
    if (i > 0) {
      json += ",";
    }
    json += '"';
    json += device_config::channelLabel(i);
    json += '"';
  }
  json += "]";
  return json;
}

String buildSwitchLabelsJson() {
  String json = "[";
  for (uint8_t i = 0; i < device_config::kSwitchLabelCount; ++i) {
    if (i > 0) {
      json += ",";
    }
    json += '"';
    json += device_config::switchLabel(i);
    json += '"';
  }
  json += "]";
  return json;
}

String buildStatusPayload() {
  const String hostname = WiFi.getHostname();
  const String ip = WiFi.localIP().toString();
  const String mdnsHost = hostname + ".local";

#if defined(ESP32)
  const uint32_t totalRam = ESP.getHeapSize();
#elif defined(ESP8266)
  const uint32_t totalRam = 81920;
#endif
  const uint32_t freeRam = ESP.getFreeHeap();
  const uint32_t usedRam = totalRam - freeRam;
  const uint32_t totalFlash = ESP.getFlashChipSize();
  const uint32_t usedFlash = ESP.getSketchSize();

  String json = String("{\n");
  json += String("  \"hostname\":\"") + hostname + "\",\n";
  json += String("  \"ip\":\"") + ip + "\",\n";
  json += String("  \"mdns\":\"") + mdnsHost + "\",\n";
  json += String("  \"swVersion\":\"") + google_sheet::getSoftwareVersion() + "\",\n";
  json += String("  \"buildDate\":\"") + __DATE__ + "\",\n";
  json += String("  \"buildTime\":\"") + __TIME__ + "\",\n";
  json += String("  \"wifiSSID\":\"") + String(wifi_manager::getWifiSsid()) + "\",\n";
  json += "  \"wifiPassword\":\"" + String(wifi_manager::getWifiPassword()) + "\",\n";
  json += "  \"apName\":\"" + String(wifi_manager::getAccessPointName()) + "\",\n";
  json += "  \"apPassword\":\"" + String(wifi_manager::getAccessPointPassword()) + "\",\n";
  json += "  \"ramTotal\":" + String(totalRam) + ",\n";
  json += "  \"ramUsed\":" + String(usedRam) + ",\n";
  json += "  \"flashTotal\":" + String(totalFlash) + ",\n";
  json += "  \"flashUsed\":" + String(usedFlash) + ",\n";
  json += "  \"channelLabels\":" + buildChannelLabelsJson() + ",\n";
  json += "  \"ledLabels\":" + buildChannelLabelsJson() + ",\n";
  json += "  \"switchLabels\":" + buildSwitchLabelsJson() + ",\n";
  json += "  \"leds\":[";
  for (uint8_t i = 0; i < status_led::kLedCount; ++i) {
    if (i > 0) {
      json += ",";
    }
    json += status_led::isOn(i) ? "true" : "false";
  }
  json += "],\n";
  json += "  \"switches\":[";
  for (uint8_t i = 0; i < gpio_switch::kSwitchCount; ++i) {
    if (i > 0) {
      json += ",";
    }
    json += gpio_switch::isPressed(i) ? "true" : "false";
  }
  json += "]\n";
  json += "}";
  return json;
}

void handleIncomingMessage(char* topic, byte* payload, unsigned int length) {
  String message;
  for (unsigned int i = 0; i < length; ++i) {
    message += static_cast<char>(payload[i]);
  }

  String topicStr = String(topic);
  String prefix = String(topicPrefix);

  String ledTopic = prefix + "/led/set";
  String stateTopic = prefix + "/led/state";
  String allTopic = prefix + "/device";

  if (topicStr == ledTopic) {
    const uint8_t ledIndex = message.toInt();
    if (ledIndex < status_led::kLedCount) {
      status_led::setOn(ledIndex, !status_led::isOn(ledIndex));
      publishStatus();
      client.publish(stateTopic.c_str(), buildStatusPayload().c_str(), true);
    }
    return;
  }

  if (topicStr == allTopic) {
    if (message.indexOf("on") >= 0 || message.indexOf("ON") >= 0 || message == "1") {
      for (uint8_t i = 0; i < status_led::kLedCount; ++i) {
        status_led::setOn(i, true);
      }
    } else if (message.indexOf("off") >= 0 || message.indexOf("OFF") >= 0 || message == "0") {
      for (uint8_t i = 0; i < status_led::kLedCount; ++i) {
        status_led::setOn(i, false);
      }
    }
    publishStatus();
    return;
  }
}

void reconnect() {
  while (!client.connected()) {
    Serial.print("Attempting MQTT connection...");
    String clientId = "esp32-ota-";
    clientId += String(random(0xffff), HEX);

    const bool connected = mqttUsername[0] != '\0'
        ? client.connect(clientId.c_str(), mqttUsername, mqttPassword)
        : client.connect(clientId.c_str());

    if (connected) {
      Serial.println("connected");
      String ledSetTopic = String(topicPrefix) + "/led/set";
      String stateTopic = String(topicPrefix) + "/state";
      String deviceTopic = String(topicPrefix) + "/device";
      client.subscribe(ledSetTopic.c_str());
      client.subscribe(deviceTopic.c_str());
      client.publish(stateTopic.c_str(), buildStatusPayload().c_str(), true);
    } else {
      Serial.print("failed, rc=");
      Serial.print(client.state());
      Serial.println(" retrying in 5 seconds");
      delay(5000);
    }
  }
}

}  // namespace

void setBroker(const char* host, uint16_t port, const char* username, const char* password) {
  if (host != nullptr && host[0] != '\0') {
    strncpy(brokerHost, host, sizeof(brokerHost) - 1);
    brokerHost[sizeof(brokerHost) - 1] = '\0';
  }
  brokerPort = port;

  if (username != nullptr) {
    strncpy(mqttUsername, username, sizeof(mqttUsername) - 1);
    mqttUsername[sizeof(mqttUsername) - 1] = '\0';
  } else {
    mqttUsername[0] = '\0';
  }

  if (password != nullptr) {
    strncpy(mqttPassword, password, sizeof(mqttPassword) - 1);
    mqttPassword[sizeof(mqttPassword) - 1] = '\0';
  } else {
    mqttPassword[0] = '\0';
  }
}

void setTopicPrefix(const char* prefix) {
  if (prefix != nullptr && prefix[0] != '\0') {
    strncpy(topicPrefix, prefix, sizeof(topicPrefix) - 1);
    topicPrefix[sizeof(topicPrefix) - 1] = '\0';
  }
}

bool isConnected() {
  return client.connected();
}

void begin() {
  brokerHost[0] = '\0';
  mqttUsername[0] = '\0';
  mqttPassword[0] = '\0';
  topicPrefix[0] = '\0';

  setBroker(device_config::kDefaultMqttBroker,
            device_config::kDefaultMqttPort,
            device_config::kDefaultMqttUsername,
            device_config::kDefaultMqttPassword);
  setTopicPrefix(device_config::kDefaultMqttPrefix);
  client.setServer(brokerHost, brokerPort);
  client.setCallback(handleIncomingMessage);
}

void handle() {
  if (WiFi.status() != WL_CONNECTED) {
    return;
  }

  if (!client.connected()) {
    reconnect();
  }

  client.loop();
}

void publishStatus() {
  if (WiFi.status() != WL_CONNECTED || !client.connected()) {
    return;
  }

  String topic = String(topicPrefix) + "/state";
  client.publish(topic.c_str(), buildStatusPayload().c_str(), true);
}

}  // namespace mqtt_manager

#else

namespace mqtt_manager {

void setBroker(const char*, uint16_t, const char*, const char*) {}
void setTopicPrefix(const char*) {}
bool isConnected() { return false; }
void begin() {}
void handle() {}
void publishStatus() {}

}  // namespace mqtt_manager

#endif

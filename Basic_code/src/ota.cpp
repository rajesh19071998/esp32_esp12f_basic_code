#include "ota.h"

#ifndef ENABLE_OTA
#define ENABLE_OTA 0
#endif

#ifndef ENABLE_WEB_UI
#define ENABLE_WEB_UI 0
#endif

#ifndef ENABLE_ALEXA
#define ENABLE_ALEXA 0
#endif

#include <Arduino.h>
#if defined(ESP32)
#if ENABLE_OTA
#include <LittleFS.h>
#include <Update.h>
#endif
#include <WebServer.h>
#elif defined(ESP8266)
#if ENABLE_OTA
#include <LittleFS.h>
#include <Updater.h>
#endif
#include <ESP8266WebServer.h>
using WebServer = ESP8266WebServer;
#endif

#include "alexa_manager.h"
#if ENABLE_OTA
#include "device_config.h"
#include "ota_web_ui.h"
#endif
#if ENABLE_WEB_UI
#include "web_dashboard_ui.h"
#include "web_led_control.h"
#endif
#include "wifi_manager.h"

// Hosts the shared HTTP server used by the dashboard, OTA flow, and Alexa fallback API.

namespace ota {
namespace {

WebServer server(80);
#if ENABLE_OTA
bool restartRequested = false;
unsigned long restartAtMs = 0;
bool otaAuthenticated = false;
String updateError;
String uploadMode = "firmware";
File littleFsUploadFile;

void handleUploadMode() {
  if (server.hasArg("type")) {
    const String selectedType = server.arg("type");
    uploadMode = (selectedType == "littlefs") ? "littlefs" : "firmware";
  }
  server.send(200, "text/plain", uploadMode);
}

bool isAuthenticated() {
  if (otaAuthenticated) {
    return true;
  }

  if (!server.hasHeader("Cookie")) {
    return false;
  }

  const String cookieHeader = server.header("Cookie");
  return cookieHeader.indexOf("ota_auth=1") >= 0;
}

void sendLoginPage(const String& errorMessage = "") {
  String html = OTA_LOGIN_UI;
  if (!errorMessage.isEmpty()) {
    html.replace("{{ERROR}}", errorMessage);
  } else {
    html.replace("{{ERROR}}", "");
  }
  server.send(200, "text/html", html);
}

void handleLoginPage() {
  sendLoginPage();
}

String trimCredential(const String& value) {
  String trimmed = value;
  trimmed.trim();
  return trimmed;
}

void handleLogin() {
  const String username = server.hasArg("username") ? trimCredential(server.arg("username")) : "";
  const String password = server.hasArg("password") ? trimCredential(server.arg("password")) : "";

  Serial.printf("OTA login attempt: username='%s', password='%s'\n", username.c_str(), password.c_str());

  if (username == device_config::kDefaultOtaUsername && password == device_config::kDefaultOtaPassword) {
    otaAuthenticated = true;
    server.sendHeader("Set-Cookie", "ota_auth=1; Path=/; Max-Age=3600; SameSite=Lax");
    server.sendHeader("Location", "/update");
    server.send(302, "text/plain", "Redirecting...");
    return;
  }

  otaAuthenticated = false;
  sendLoginPage("Invalid username or password.");
}

void handleUpdatePage() {
  if (!isAuthenticated()) {
    sendLoginPage();
    return;
  }
  server.send(200, "text/html", OTA_WEB_UI);
}

void handleUpdateResult() {
  if (!isAuthenticated()) {
    server.send(401, "text/plain", "Unauthorized");
    return;
  }

  if (uploadMode == "littlefs") {
    if (!updateError.isEmpty()) {
      server.send(500, "text/plain", updateError);
      return;
    }
    restartRequested = true;
    restartAtMs = millis() + 1000;
    server.send(200, "text/plain", "LittleFS update complete. Rebooting device.");
    return;
  }

  if (Update.hasError()) {
    const String message = updateError.isEmpty() ? "Firmware update failed." : updateError;
    server.send(500, "text/plain", message);
    return;
  }

  restartRequested = true;
  restartAtMs = millis() + 1500;
  server.send(200, "text/plain", "Firmware update complete. Rebooting device.");
}

void handleUpdateUpload() {
  if (!isAuthenticated()) {
    server.send(401, "text/plain", "Unauthorized");
    return;
  }

  HTTPUpload& upload = server.upload();

  switch (upload.status) {
    case UPLOAD_FILE_START: {
      updateError = "";
      Serial.printf("Upload start: %s [%s]\n", upload.filename.c_str(), uploadMode.c_str());

      if (uploadMode == "littlefs") {
#if defined(ESP32) || defined(ESP8266)
#if defined(ESP32)
        if (!LittleFS.begin(true)) {
#elif defined(ESP8266)
        if (!LittleFS.begin()) {
#endif
          updateError = "Unable to initialize LittleFS.";
          break;
        }
        if (littleFsUploadFile) {
          littleFsUploadFile.close();
        }
        littleFsUploadFile = LittleFS.open("/littlefs-upload.bin", "w");
        if (!littleFsUploadFile) {
          updateError = "Unable to open LittleFS target.";
        }
#endif
        break;
      }

#if defined(ESP32)
      if (!Update.begin(UPDATE_SIZE_UNKNOWN)) {
#elif defined(ESP8266)
      if (!Update.begin((ESP.getFreeSketchSpace() - 0x1000) & 0xFFFFF000)) {
#endif
        Update.printError(Serial);
        updateError = "Unable to start OTA update.";
      }
      break;
    }

    case UPLOAD_FILE_WRITE: {
      if (uploadMode == "littlefs") {
#if defined(ESP32) || defined(ESP8266)
        if (littleFsUploadFile && upload.buf != nullptr && upload.currentSize > 0) {
          const size_t written = littleFsUploadFile.write(upload.buf, upload.currentSize);
          if (written != upload.currentSize) {
            updateError = "LittleFS write failed during upload.";
          }
        }
#endif
        break;
      }

      if (Update.hasError()) {
        break;
      }

      const size_t written = Update.write(upload.buf, upload.currentSize);
      if (written != upload.currentSize) {
        Update.printError(Serial);
        updateError = "Flash write failed during OTA upload.";
      }
      break;
    }

    case UPLOAD_FILE_END: {
      if (uploadMode == "littlefs") {
#if defined(ESP32) || defined(ESP8266)
        if (littleFsUploadFile) {
          littleFsUploadFile.close();
        }
        if (updateError.isEmpty()) {
          Serial.printf("LittleFS upload complete: %u bytes\n", upload.totalSize);
        }
#endif
        break;
      }

      if (Update.hasError()) {
        break;
      }

      if (!Update.end(true)) {
        Update.printError(Serial);
        updateError = "Firmware validation failed at the end of OTA upload.";
      } else {
        Serial.printf("OTA upload complete: %u bytes\n", upload.totalSize);
      }
      break;
    }

    case UPLOAD_FILE_ABORTED:
      if (uploadMode == "littlefs") {
#if defined(ESP32) || defined(ESP8266)
        if (littleFsUploadFile) {
          littleFsUploadFile.close();
        }
#endif
      } else {
#if defined(ESP32)
        Update.abort();
#elif defined(ESP8266)
        Update.end();
#endif
      }
      updateError = "OTA upload aborted by the client.";
      Serial.println(updateError);
      break;

    default:
      break;
  }
}
#endif

#if ENABLE_WEB_UI
void handleRoot() {
  server.send(200, "text/html", webDashboardUi());
#if ENABLE_OTA
  otaAuthenticated = false;
#endif
}
#endif

void handleNotFound() {
  if (alexa_manager::handleApiCall(server.uri(), server.arg(0))) {
    return;
  }
  server.send(404, "text/plain", "Not found");
}

void configureRoutes() {
#if ENABLE_WEB_UI
  server.on("/", HTTP_GET, handleRoot);
  server.on("/dashboard", HTTP_GET, handleRoot);
  web_led_control::registerRoutes(server);
#endif
#if ENABLE_OTA
  server.on("/login", HTTP_GET, handleLoginPage);
  server.on("/login", HTTP_POST, handleLogin);
  server.on("/upload-mode", HTTP_POST, handleUploadMode);
  server.on("/update", HTTP_GET, handleUpdatePage);
  server.on("/update", HTTP_POST, handleUpdateResult, handleUpdateUpload);
#endif
  server.onNotFound(handleNotFound);
  if (!alexa_manager::begin(server)) {
    server.begin();
  }
  Serial.println("HTTP OTA server started");
}

}  // namespace

void begin() {
  wifi_manager::begin();
#if ENABLE_WEB_UI || ENABLE_OTA || ENABLE_ALEXA
  configureRoutes();
#endif
}

void handle() {
#if ENABLE_WEB_UI || ENABLE_OTA || ENABLE_ALEXA
  server.handleClient();
#endif

#if ENABLE_OTA
  if (restartRequested && millis() >= restartAtMs) {
    Serial.println("Rebooting into updated firmware");
    ESP.restart();
  }
#endif
}

}  // namespace ota
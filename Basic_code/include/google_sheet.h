#pragma once

#include <Arduino.h>

// Reports device metadata and memory usage to a Google Apps Script endpoint.
namespace google_sheet {

// Overrides the default reporting endpoint.
void setEndpoint(const char* url);

// Sends the current device information to the configured endpoint.
void sendDeviceInfo();

// Returns the firmware version string compiled into the build.
const char* getSoftwareVersion();

// Returns the compile date used for the current firmware image.
const char* getBuildDate();

// Returns the compile time used for the current firmware image.
const char* getBuildTime();

// Builds a JSON document describing the board, network, and LED state.
String getDeviceInfoJson();

// Builds a JSON document with current memory usage details.
String getMemoryInfoJson();

}  // namespace google_sheet

// WiFiConnector.cpp
#include "WiFiConnector.h"
#include <WiFi.h>
#include <WiFiManager.h>

WiFiConnector::WiFiConnector() {}

void WiFiConnector::begin() {
  // Use WiFiManager autoConnect (blocks until configured first time).
  // If you want non-blocking behavior on dev boards, replace with setConfigPortalBlocking(false)
  WiFiManager wm;
  wm.autoConnect("ESP32-Color-Setup");
}

bool WiFiConnector::connected() {
  return WiFi.status() == WL_CONNECTED;
}

void WiFiConnector::loop() {
  // nothing to do for now; WiFiManager handles captive portal in background
}

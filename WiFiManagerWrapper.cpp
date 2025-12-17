// WiFiManagerWrapper.cpp
#include "WiFiManagerWrapper.h"

WiFiManagerWrapper::WiFiManagerWrapper() : wifi() {}

void WiFiManagerWrapper::connect() {
  if(!wifi.connected()) wifi.begin();
}

bool WiFiManagerWrapper::isConnected() {
  return wifi.connected();
}

void WiFiManagerWrapper::loop() {
  wifi.loop();
}

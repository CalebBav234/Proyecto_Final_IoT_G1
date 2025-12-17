// WiFiManagerWrapper.h
#ifndef WIFI_MANAGER_WRAPPER_H
#define WIFI_MANAGER_WRAPPER_H

#include "WiFiConnector.h"

class WiFiManagerWrapper {
public:
  WiFiManagerWrapper();
  void connect();
  bool isConnected();
  void loop();
private:
  WiFiConnector wifi;
};

#endif // WIFI_MANAGER_WRAPPER_H

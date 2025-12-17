// WiFiConnector.h
#ifndef WIFI_CONNECTOR_H
#define WIFI_CONNECTOR_H

class WiFiConnector {
public:
  WiFiConnector();
  void begin();         // start provisioning portal if needed
  bool connected();     // true if WiFi connected
  void loop();          // call regularly to handle captive portal (non-blocking)
};

#endif // WIFI_CONNECTOR_H

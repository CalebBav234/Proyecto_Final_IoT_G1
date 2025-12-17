#ifndef SHADOW_CLIENT_H
#define SHADOW_CLIENT_H

#include <Arduino.h>
#include <functional>
#include <WiFiClientSecure.h>
#include <PubSubClient.h>
#include <ArduinoJson.h>
#include "DeviceConfig.h"
#include "ColorSensor.h"

class ShadowClient {
public:
  ShadowClient();

  // Connect to AWS IoT (uses certs from DeviceConfig.h)
  void begin();
  bool connected();

  // Call regularly in loop()
  void loop();

  // Register callbacks invoked on incoming messages
  void onDesiredColor(std::function<void(const String&)> cb);
  void onScheduleUpdate(std::function<void(int,int)> cb);
  void onCommand(std::function<void(const String&)> cb);

  // Publishers (reported)
  void publishReportedConfig(int pillHour, int pillMinute, bool buzzerEnabled);
  void publishDispenseReport(const String& dispensedColor,
                             int dispensedAngle,
                             const RGB& rgb,
                             const String& status,
                             unsigned long command_id);

  // Clear desired keys (set them to null)
  void clearDesired();

private:
  WiFiClientSecure _net;
  PubSubClient _mqtt;

  String _updateTopic;
  String _deltaTopic;
  String _commandTopic;

  // user callbacks
  std::function<void(const String&)> _cbDesiredColor = nullptr;
  std::function<void(int,int)> _cbSchedule = nullptr;
  std::function<void(const String&)> _cbCommand = nullptr;

  // internal helpers
  void connectIfNeeded();
  void handleDeltaPayload(const String& payload);
  static void mqttCallbackStatic(char* topic, byte* payload, unsigned int length);
  void mqttCallback(char* topic, byte* payload, unsigned int length);

  // single-instance pointer used by static callback (one device -> OK)
  static ShadowClient* s_instance;
};

#endif // SHADOW_CLIENT_H

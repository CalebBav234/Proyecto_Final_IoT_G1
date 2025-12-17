// AppController.h
#ifndef APP_CONTROLLER_H
#define APP_CONTROLLER_H

#include <Arduino.h>
#include "WiFiManagerWrapper.h"
#include "ColorSensor.h"
#include "ServoController.h"
#include "Buzzer.h"
#include "ShadowClient.h"
#include "CommandHandler.h"
#include "DeviceConfig.h"

class AppController {
public:
  AppController();

  // initialize hardware, network, shadow, handlers
  void begin();

  // main loop
  void loop();

private:
  WiFiManagerWrapper _wifi;
  ColorSensor _sensor;
  ServoController _servo;
  Buzzer _buzzer;
  ShadowClient _shadow;
  CommandHandler _cmdHandler;

  void initTime(); // NTP + timezone
};

#endif // APP_CONTROLLER_H

// CommandHandler.h
#ifndef COMMAND_HANDLER_H
#define COMMAND_HANDLER_H

#include <Arduino.h>
#include "ColorSensor.h"
#include "ServoController.h"
#include "Buzzer.h"
#include "ShadowClient.h"
#include "DeviceConfig.h"
#include <time.h>

class CommandHandler {
public:
  CommandHandler(ServoController &servo,
                 Buzzer &buzzer,
                 ColorSensor &sensor,
                 ShadowClient &shadow);

  // Handle immediate/desired actions
  void handleCommandPayload(const String &payloadJson); // raw JSON from esp32/commands/<THING>
  void handleDesiredColor(const String &color);        // called from Shadow delta (compat)
  void handleScheduleUpdate(int hour, int minute);     // called from Shadow delta for config

  // Must be called frequently from main loop
  void loop();

private:
  ServoController &_servo;
  Buzzer &_buzzer;
  ColorSensor &_sensor;
  ShadowClient &_shadow;

  int _pillHour = -1;
  int _pillMinute = -1;
  bool _buzzerEnabled = false;
  bool _alarmTriggered = false;

  unsigned long lastHandledCommandId = 0;

  // helpers
  int colorToAngle(const String &color);
  void performDispense(const String &color, unsigned long command_id);
  unsigned long nowEpochLocal() const; // Bolivia-adjusted epoch
};

#endif // COMMAND_HANDLER_H

// CommandHandler.cpp
#include "CommandHandler.h"
#include <ArduinoJson.h>

CommandHandler::CommandHandler(ServoController &servo,
                               Buzzer &buzzer,
                               ColorSensor &sensor,
                               ShadowClient &shadow)
  : _servo(servo), _buzzer(buzzer), _sensor(sensor), _shadow(shadow) {}

int CommandHandler::colorToAngle(const String &color) {
  if (color.equalsIgnoreCase("WHITE")) return 0;
  if (color.equalsIgnoreCase("CREAM")) return 30;
  if (color.equalsIgnoreCase("BROWN")) return 60;
  if (color.equalsIgnoreCase("RED"))   return 90;
  if (color.equalsIgnoreCase("BLUE"))  return 120;
  if (color.equalsIgnoreCase("GREEN")) return 150;
  return 180; // other / fallback
}

unsigned long CommandHandler::nowEpochLocal() const {
  // time(nullptr) returns UTC epoch; apply BOLIVIA_OFFSET_SECONDS for local epoch
  time_t t = time(nullptr);
  return (unsigned long)( (long)t + BOLIVIA_OFFSET_SECONDS );
}

void CommandHandler::performDispense(const String &color, unsigned long command_id) {
  // ignore duplicate command ids
  if (command_id != 0 && command_id == lastHandledCommandId) {
    Serial.println("[CommandHandler] Duplicate command_id - skipping");
    return;
  }
  if (command_id != 0) lastHandledCommandId = command_id;

  Serial.printf("[CommandHandler] Dispense requested: %s id=%lu\n", color.c_str(), command_id);

  int angle = colorToAngle(color);
  _servo.move(angle);

  // beep while dispensing (non-blocking start)
  _buzzer.beep(800);

  // read color after movement
  delay(250); // small settle time
  RGB measured = _sensor.readNormalizedRGB();

  // publish report to shadow (reported state) and include Bolivia epoch
  _shadow.publishDispenseReport(color, angle, measured, String("OK"), command_id);

  // clear desired to avoid repeated delta for same command (best-effort)
  _shadow.clearDesired();

  // return servo to HOME position (safe)
  _servo.move(90);
}

void CommandHandler::handleCommandPayload(const String &payloadJson) {
  // Expect JSON like: {"action":"dispense","color":"RED","command_id":12345}
  StaticJsonDocument<512> doc;
  DeserializationError err = deserializeJson(doc, payloadJson);
  if (err) {
    Serial.print("[CommandHandler] command JSON parse error: ");
    Serial.println(err.c_str());
    return;
  }

  String action = doc["action"] | "";
  if (action.equalsIgnoreCase("dispense")) {
    String color = doc["color"] | "";
    unsigned long cmdid = (unsigned long)(doc["command_id"] | 0UL);
    if (color.length() == 0) {
      Serial.println("[CommandHandler] dispense command missing color");
      return;
    }
    performDispense(color, cmdid);
  } else {
    Serial.printf("[CommandHandler] unknown action: %s\n", action.c_str());
  }
}

void CommandHandler::handleDesiredColor(const String &color) {
  // Shadow desired.color is accepted as a convenience (backwards compatible).
  // But primary design: cloud should publish to esp32/commands/<THING>.
  Serial.printf("[CommandHandler] desired.color via Shadow -> %s\n", color.c_str());
  performDispense(color, 0UL);
}

void CommandHandler::handleScheduleUpdate(int hour, int minute) {
  Serial.printf("[CommandHandler] Received schedule update %02d:%02d\n", hour, minute);
  _pillHour = hour;
  _pillMinute = minute;

  // publish reported config back to Shadow so cloud knows device accepted config
  _shadow.publishReportedConfig(_pillHour, _pillMinute, _buzzerEnabled);
}

void CommandHandler::loop() {
  // check buzzer timeout via its class
  _buzzer.loop();

  // check schedule alarm using local time
  time_t t = time(nullptr);
  if (t == (time_t)0) return; // time not yet available
  struct tm localtm;
  // convert to local (apply BOLIVIA offset)
  time_t local_epoch = (time_t)((long)t + BOLIVIA_OFFSET_SECONDS);
  gmtime_r(&local_epoch, &localtm); // gmtime_r gives UTC for given epoch; we used adjusted epoch so it represents local time

  int hour = localtm.tm_hour;
  int minute = localtm.tm_min;

  if (_buzzerEnabled && _pillHour >= 0 && _pillMinute >= 0) {
    if (hour == _pillHour && minute == _pillMinute && !_alarmTriggered) {
      Serial.println("[CommandHandler] Scheduled pill time reached -> alarm");
      _buzzer.beep(5000);
      _alarmTriggered = true;
    }
    if (minute != _pillMinute) _alarmTriggered = false;
  }
}

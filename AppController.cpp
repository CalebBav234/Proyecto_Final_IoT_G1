// AppController.cpp
#include "AppController.h"
#include <time.h>

AppController::AppController()
  : _wifi(),
    _sensor(PIN_S0, PIN_S1, PIN_S2, PIN_S3, PIN_OUT),
    _servo(PIN_SERVO),
    _buzzer(PIN_BUZZER),
    _shadow(),
    _cmdHandler(_servo, _buzzer, _sensor, _shadow) {}

void AppController::initTime() {
  // Configure NTP with Bolivia offset (BOLIVIA_OFFSET_SECONDS from DeviceConfig)
  // configTime expects gmtOffset_sec, daylightOffset_sec, servers...
  configTime(BOLIVIA_OFFSET_SECONDS, 0, "pool.ntp.org", "time.google.com");
  Serial.println("[AppController] NTP configured (Bolivia UTC-4)");
}

void AppController::begin() {
  Serial.begin(115200);
  delay(200);

  Serial.println("[AppController] Begin");

  // hardware init
  _sensor.begin();
  _servo.begin();
  _buzzer.begin();

  // WiFi (WiFiManager will open provisioning portal on first boot)
  _wifi.connect();
  while (!_wifi.isConnected()) {
    Serial.print("[AppController] Waiting for WiFi...");
    delay(500);
  }
  Serial.println("[AppController] WiFi connected");

  // init time
  initTime();

  // Shadow/MQTT
  _shadow.begin();

  // register callbacks
  _shadow.onCommand([this](const String &payload) {
    _cmdHandler.handleCommandPayload(payload);
  });

  _shadow.onDesiredColor([this](const String &color) {
    _cmdHandler.handleDesiredColor(color);
  });

  _shadow.onScheduleUpdate([this](int h, int m) {
    _cmdHandler.handleScheduleUpdate(h, m);
  });

  Serial.println("[AppController] System ready");
}

void AppController::loop() {
  _wifi.loop();
  _shadow.loop();
  _cmdHandler.loop();

  delay(10);
}

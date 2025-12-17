#ifndef SERVO_CONTROLLER_H
#define SERVO_CONTROLLER_H

#include <Arduino.h>
#include <ESP32Servo.h>

class ServoController {
public:
    ServoController(int pin);

    void begin();
    void move(int targetAngle);
    int getAngle();

private:
    Servo _servo;
    int _pin;
    int _currentAngle;
};

#endif

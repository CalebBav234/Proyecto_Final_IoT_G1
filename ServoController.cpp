#include "ServoController.h"

ServoController::ServoController(int pin)
    : _pin(pin), _currentAngle(90) {}

void ServoController::begin() {
    _servo.attach(_pin);
    _servo.write(_currentAngle);
}

void ServoController::move(int targetAngle) {
    targetAngle = constrain(targetAngle, 0, 180);

    int step = (targetAngle > _currentAngle) ? 1 : -1;

    while (_currentAngle != targetAngle) {
        _currentAngle += step;
        _servo.write(_currentAngle);
        delay(12);  // smooth movement
    }
}

int ServoController::getAngle() {
    return _currentAngle;
}

#include "Buzzer.h"

Buzzer::Buzzer(int pin)
    : _pin(pin), _isBeeping(false), _endTime(0) {}

void Buzzer::begin() {
    pinMode(_pin, OUTPUT);
    digitalWrite(_pin, LOW);
}

void Buzzer::beep(unsigned long durationMs) {
    digitalWrite(_pin, HIGH);
    _isBeeping = true;
    _endTime = millis() + durationMs;
}

void Buzzer::loop() {
    if (_isBeeping && millis() >= _endTime) {
        digitalWrite(_pin, LOW);
        _isBeeping = false;
    }
}

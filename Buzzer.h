#ifndef BUZZER_H
#define BUZZER_H

#include <Arduino.h>

class Buzzer {
public:
    Buzzer(int pin);

    void begin();
    void beep(unsigned long durationMs);
    void loop();

private:
    int _pin;
    bool _isBeeping;
    unsigned long _endTime;
};

#endif

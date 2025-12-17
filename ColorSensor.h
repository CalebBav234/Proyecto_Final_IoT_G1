#ifndef COLOR_SENSOR_H
#define COLOR_SENSOR_H

#include <Arduino.h>
#include "DeviceConfig.h"

struct RGB {
    int r;
    int g;
    int b;
};

class ColorSensor {
public:
    ColorSensor(int s0, int s1, int s2, int s3, int outPin);

    void begin();
    RGB readRawRGB();
    RGB readNormalizedRGB();   // scaled 0–255

private:
    int _s0, _s1, _s2, _s3, _out;
    int readColorFrequency(bool s2State, bool s3State);
};

#endif

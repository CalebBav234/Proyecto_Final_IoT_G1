#include "ColorSensor.h"

ColorSensor::ColorSensor(int s0, int s1, int s2, int s3, int outPin)
    : _s0(s0), _s1(s1), _s2(s2), _s3(s3), _out(outPin) {}

void ColorSensor::begin() {
    pinMode(_s0, OUTPUT);
    pinMode(_s1, OUTPUT);
    pinMode(_s2, OUTPUT);
    pinMode(_s3, OUTPUT);
    pinMode(_out, INPUT);

    // Set scaling to 20% (stable for most ESP32 boards)
    digitalWrite(_s0, HIGH);
    digitalWrite(_s1, LOW);
}

int ColorSensor::readColorFrequency(bool s2State, bool s3State) {
    digitalWrite(_s2, s2State ? HIGH : LOW);
    digitalWrite(_s3, s3State ? HIGH : LOW);

    delayMicroseconds(200);

    int pulse = pulseIn(_out, LOW, 25000);
    if (pulse == 0) pulse = 1;  // avoid division by zero

    return 1000000 / pulse;     // frequency
}

RGB ColorSensor::readRawRGB() {
    int r = readColorFrequency(LOW, LOW);   // Red
    int g = readColorFrequency(HIGH, HIGH); // Green
    int b = readColorFrequency(LOW, HIGH);  // Blue

    return { r, g, b };
}

RGB ColorSensor::readNormalizedRGB() {
    RGB raw = readRawRGB();

    int maxVal = max(raw.r, max(raw.g, raw.b));
    if (maxVal == 0) maxVal = 1;

    return {
        map(raw.r, 0, maxVal, 0, 255),
        map(raw.g, 0, maxVal, 0, 255),
        map(raw.b, 0, maxVal, 0, 255)
    };
}

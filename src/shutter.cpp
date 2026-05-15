#include "shutter.h"

void Shutter::begin(int pin, int openAngle, int closeAngle,
                    int minPulse, int maxPulse, int hz) {
    _pin        = pin;
    _openAngle  = openAngle;
    _closeAngle = closeAngle;

    _servo.setPeriodHertz(hz);
    _servo.attach(_pin, minPulse, maxPulse);
    _servo.write(_closeAngle);
    _currentAngle = _closeAngle;
    _isOpen = false;
    _autoClosePending = false;

    Serial.printf("[Shutter] begin pin=%d open=%d close=%d\n", _pin, _openAngle, _closeAngle);
}

void Shutter::open() {
    _servo.write(_openAngle);
    _currentAngle = _openAngle;
    _isOpen = true;
    _autoClosePending = false;
    Serial.printf("[Shutter] -> OPEN (%d deg)\n", _openAngle);
}

void Shutter::close() {
    _servo.write(_closeAngle);
    _currentAngle = _closeAngle;
    _isOpen = false;
    _autoClosePending = false;
    Serial.printf("[Shutter] -> CLOSE (%d deg)\n", _closeAngle);
}

void Shutter::snapshot(unsigned long durationMs) {
    _servo.write(_openAngle);
    _currentAngle = _openAngle;
    _isOpen = true;
    _autoCloseStartMs = millis();
    _autoCloseDurationMs = durationMs;
    _autoClosePending = true;
    Serial.printf("[Shutter] SNAPSHOT open (%d deg), auto-close in %lu ms\n",
                  _openAngle, durationMs);
}

void Shutter::loop() {
    if (_autoClosePending && millis() - _autoCloseStartMs >= _autoCloseDurationMs) {
        close();
        Serial.printf("[Shutter] SNAPSHOT auto-close (%d deg)\n", _closeAngle);
    }
}

int  Shutter::currentAngle() const { return _currentAngle; }
bool Shutter::isOpen()       const { return _isOpen; }
int  Shutter::openAngle()    const { return _openAngle; }
int  Shutter::closeAngle()   const { return _closeAngle; }

void Shutter::setOpenAngle(int angle) {
    _openAngle = angle;
    Serial.printf("[Shutter] openAngle set to %d deg\n", angle);
}

void Shutter::setCloseAngle(int angle) {
    _closeAngle = angle;
    Serial.printf("[Shutter] closeAngle set to %d deg\n", angle);
}

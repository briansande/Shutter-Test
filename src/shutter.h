#pragma once
#include <Arduino.h>
#include <ESP32Servo.h>

class Shutter {
public:
    void begin(int pin, int openAngle, int closeAngle,
               int minPulse = 500, int maxPulse = 2400, int hz = 50);
    void open();
    void close();
    void snapshot(unsigned long durationMs);
    void loop();

    int  currentAngle() const;
    bool isOpen()        const;
    int  openAngle()     const;
    int  closeAngle()    const;

    void setOpenAngle(int angle);
    void setCloseAngle(int angle);

private:
    Servo         _servo;
    int           _pin        = -1;
    int           _openAngle  = 106;
    int           _closeAngle = 74;
    int           _currentAngle = 74;
    bool          _isOpen     = false;
    bool          _autoClosePending = false;
    unsigned long _autoCloseStartMs = 0;
    unsigned long _autoCloseDurationMs = 0;
};

#pragma once
#include <Arduino.h>

namespace config {
    static const int SHUTTER_SERVO_PIN = 27;

    static const int SHUTTER_OPEN_ANGLE  = 106;
    static const int SHUTTER_CLOSE_ANGLE = 74;

    static const int SERVO_MIN_PULSE = 500;
    static const int SERVO_MAX_PULSE = 2400;
    static const int SERVO_HZ        = 50;

    static const char* NVS_NAMESPACE = "shutter";
    static const char* NVS_KEY_OPEN  = "openAngle";
    static const char* NVS_KEY_CLOSE = "closeAngle";

    static const uint16_t WEB_PORT = 80;
}

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

    namespace feeder {
        static const int IN1_PIN = 26;
        static const int IN2_PIN = 25;
        static const int IN3_PIN = 33;
        static const int IN4_PIN = 32;

        static const int STEPS_PER_REV = 4096;
        static const int DEFAULT_FEED_ROTATIONS = 1;
        static const int STEP_DELAY_MS = 2;

        static const char* NVS_NAMESPACE   = "feeder";
        static const char* NVS_KEY_ROT     = "feedRot";
        static const char* NVS_KEY_AUTO    = "autoFeed";
        static const int  JOG_STEPS        = 128;
    }
}

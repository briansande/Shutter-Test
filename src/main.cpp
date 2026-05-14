#include <Arduino.h>
#include "config.h"
#include "settings.h"
#include "shutter.h"
#include "web.h"
#include "wifi_manager.h"
#include "wifi_secrets.h"

static Shutter      shutter;
static WebInterface web;

void setup() {
    Serial.begin(115200);
    delay(500);

    int openAngle  = settings::loadInt(config::NVS_NAMESPACE, config::NVS_KEY_OPEN,  config::SHUTTER_OPEN_ANGLE);
    int closeAngle = settings::loadInt(config::NVS_NAMESPACE, config::NVS_KEY_CLOSE, config::SHUTTER_CLOSE_ANGLE);

    shutter.begin(config::SHUTTER_SERVO_PIN, openAngle, closeAngle,
                  config::SERVO_MIN_PULSE, config::SERVO_MAX_PULSE, config::SERVO_HZ);

    wifi::connect(WIFI_SSID, WIFI_PASS);
    web.begin(shutter, config::WEB_PORT);
}

void loop() {
    wifi::reconnectIfNeeded();
    shutter.loop();
    web.loop();
}

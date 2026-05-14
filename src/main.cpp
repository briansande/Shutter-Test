#include <Arduino.h>
#include "config.h"
#include "settings.h"
#include "shutter.h"
#include "paper_feeder.h"
#include "web.h"
#include "wifi_manager.h"
#include "wifi_secrets.h"

static Shutter      shutter;
static PaperFeeder  feeder;
static WebInterface web;

void setup() {
    Serial.begin(115200);
    delay(500);

    int openAngle  = settings::loadInt(config::NVS_NAMESPACE, config::NVS_KEY_OPEN,  config::SHUTTER_OPEN_ANGLE);
    int closeAngle = settings::loadInt(config::NVS_NAMESPACE, config::NVS_KEY_CLOSE, config::SHUTTER_CLOSE_ANGLE);

    shutter.begin(config::SHUTTER_SERVO_PIN, openAngle, closeAngle,
                  config::SERVO_MIN_PULSE, config::SERVO_MAX_PULSE, config::SERVO_HZ);

    feeder.begin(config::feeder::IN1_PIN, config::feeder::IN2_PIN,
                 config::feeder::IN3_PIN, config::feeder::IN4_PIN,
                 config::feeder::STEPS_PER_REV,
                 config::feeder::STEP_DELAY_MS,
                 config::feeder::DEFAULT_FEED_ROTATIONS);

    wifi::connect(WIFI_SSID, WIFI_PASS);
    web.begin(shutter, feeder, config::WEB_PORT);
}

void loop() {
    wifi::reconnectIfNeeded();
    shutter.loop();
    feeder.loop();
    web.loop();
}

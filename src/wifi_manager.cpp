#include "wifi_manager.h"
#include <WiFi.h>

namespace wifi {

void connect(const char* ssid, const char* pass) {
    Serial.printf("[WiFi] connecting to %s ", ssid);
    WiFi.mode(WIFI_STA);
    WiFi.begin(ssid, pass);
    while (WiFi.status() != WL_CONNECTED) {
        delay(500);
        Serial.print(".");
    }
    Serial.println();
    Serial.printf("[WiFi] connected — IP: %s\n", WiFi.localIP().toString().c_str());
}

bool isConnected() {
    return WiFi.status() == WL_CONNECTED;
}

void reconnectIfNeeded() {
    static unsigned long lastCheck = 0;
    if (millis() - lastCheck < 5000) return;
    lastCheck = millis();

    if (WiFi.status() != WL_CONNECTED) {
        Serial.println("[WiFi] connection lost, reconnecting...");
        WiFi.reconnect();
    }
}

}

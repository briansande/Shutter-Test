#include "wifi_manager.h"
#include <WiFi.h>

namespace wifi {

namespace {
    constexpr unsigned long CONNECT_TIMEOUT_MS = 15000;
    constexpr unsigned long CONNECT_POLL_MS = 250;
    constexpr unsigned long RECONNECT_INTERVAL_MS = 10000;

    const char* configuredSsid = nullptr;
    const char* configuredPass = nullptr;
    unsigned long lastReconnectAttempt = 0;

    bool hasCredentials() {
        return configuredSsid != nullptr && configuredSsid[0] != '\0';
    }

    void beginStationConnection() {
        WiFi.mode(WIFI_STA);
        WiFi.begin(configuredSsid, configuredPass);
        lastReconnectAttempt = millis();
    }
}

bool connect(const char* ssid, const char* pass) {
    configuredSsid = ssid;
    configuredPass = pass;

    if (!hasCredentials()) {
        Serial.println("[WiFi] missing SSID; skipping WiFi connection");
        return false;
    }

    Serial.printf("[WiFi] connecting to %s ", configuredSsid);
    beginStationConnection();

    const unsigned long startedAt = millis();
    while (WiFi.status() != WL_CONNECTED && millis() - startedAt < CONNECT_TIMEOUT_MS) {
        delay(CONNECT_POLL_MS);
        Serial.print(".");
    }

    Serial.println();

    if (WiFi.status() == WL_CONNECTED) {
        Serial.printf("[WiFi] connected - IP: %s\n", WiFi.localIP().toString().c_str());
        return true;
    }

    Serial.printf("[WiFi] connection timed out after %lu ms (status %d); continuing startup\n",
                  CONNECT_TIMEOUT_MS,
                  static_cast<int>(WiFi.status()));
    return false;
}

bool isConnected() {
    return WiFi.status() == WL_CONNECTED;
}

void reconnectIfNeeded() {
    if (WiFi.status() == WL_CONNECTED) return;
    if (!hasCredentials()) return;
    if (millis() - lastReconnectAttempt < RECONNECT_INTERVAL_MS) return;

    Serial.printf("[WiFi] disconnected (status %d); reconnecting\n", static_cast<int>(WiFi.status()));
    beginStationConnection();
}

}

#pragma once
#include <Arduino.h>

namespace wifi {
    bool connect(const char* ssid, const char* pass);
    bool isConnected();
    void reconnectIfNeeded();
}

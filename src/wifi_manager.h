#pragma once
#include <Arduino.h>

namespace wifi {
    void connect(const char* ssid, const char* pass);
    bool isConnected();
    void reconnectIfNeeded();
}

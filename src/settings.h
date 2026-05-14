#pragma once
#include <Arduino.h>

namespace settings {
    int  loadInt(const char* ns, const char* key, int fallback);
    void saveInt(const char* ns, const char* key, int value);
}

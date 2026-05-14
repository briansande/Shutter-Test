#include "settings.h"
#include <Preferences.h>

namespace settings {

int loadInt(const char* ns, const char* key, int fallback) {
    Preferences prefs;
    prefs.begin(ns, true);
    int val = prefs.getInt(key, fallback);
    prefs.end();
    return val;
}

void saveInt(const char* ns, const char* key, int value) {
    Preferences prefs;
    prefs.begin(ns, false);
    prefs.putInt(key, value);
    prefs.end();
}

}

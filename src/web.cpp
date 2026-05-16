#include "web.h"
#include "html.h"
#include "settings.h"
#include "config.h"
#include <WiFi.h>
#include <errno.h>
#include <stdlib.h>

namespace {
bool parseIntArg(WebServer* server, const char* name, long minValue, long maxValue, long& value) {
    if (!server->hasArg(name)) {
        return false;
    }

    String text = server->arg(name);
    text.trim();
    if (text.length() == 0) {
        return false;
    }

    errno = 0;
    char* end = nullptr;
    long parsed = strtol(text.c_str(), &end, 10);
    if (errno == ERANGE || end == text.c_str() || *end != '\0') {
        return false;
    }
    if (parsed < minValue || parsed > maxValue) {
        return false;
    }

    value = parsed;
    return true;
}
}

void WebInterface::begin(Shutter& shutter, PaperFeeder& feeder, uint16_t port) {
    _shutter = &shutter;
    _feeder  = &feeder;
    _server  = new WebServer(port);

    _autoFeedEnabled = settings::loadInt(
        config::feeder::NVS_NAMESPACE,
        config::feeder::NVS_KEY_AUTO, 0) != 0;

    _server->on("/",     HTTP_GET,  [this](){ handleRoot(); });
    _server->on("/open", HTTP_POST, [this](){ handleOpen(); });
    _server->on("/close",HTTP_POST, [this](){ handleClose(); });
    _server->on("/save", HTTP_POST, [this](){ handleSave(); });
    _server->on("/snapshot", HTTP_POST, [this](){ handleSnapshot(); });

    _server->on("/feed",              HTTP_POST, [this](){ handleFeed(); });
    _server->on("/feed/jog",          HTTP_POST, [this](){ handleFeedJog(); });
    _server->on("/feed/stop",         HTTP_POST, [this](){ handleFeedStop(); });
    _server->on("/feed/status",       HTTP_GET,  [this](){ handleFeedStatus(); });
    _server->on("/feed/settings", HTTP_GET,      [this](){ handleFeedSettingsGet(); });
    _server->on("/feed/settings", HTTP_POST,     [this](){ handleFeedSettingsSave(); });
    _server->on("/feed/auto",         HTTP_POST, [this](){ handleFeedAuto(); });

    _server->begin();
    Serial.println("[Web] server started");
}

void WebInterface::loop() {
    _server->handleClient();

    if (_autoFeedPending && !_shutter->isOpen() && !_feeder->isFeeding()) {
        if (!_autoFeedDelayStarted) {
            _autoFeedDelayStarted = true;
            _autoFeedDelayStartMs = millis();
            return;
        }

        if (millis() - _autoFeedDelayStartMs < config::feeder::AUTO_FEED_DELAY_MS) {
            return;
        }

        _autoFeedPending = false;
        _autoFeedDelayStarted = false;
        _feeder->feed(_feeder->feedRotations());
    }
}

void WebInterface::handleRoot() {
    size_t sz = sizeof(HTML_PAGE) + 192;
    char* buf = new char[sz];
    snprintf(buf, sz, HTML_PAGE,
             _shutter->openAngle(),
             _shutter->closeAngle(),
             _feeder->feedRotations(),
             _feeder->feedSpeedStepsPerSec(),
             config::feeder::JOG_STEPS,
             _autoFeedEnabled ? "checked" : "",
             WiFi.localIP().toString().c_str());
    _server->send(200, "text/html", buf);
    delete[] buf;
}

void WebInterface::handleOpen() {
    _shutter->open();
    String json = "{\"position\":\"OPEN\",\"angle\":" + String(_shutter->openAngle()) + "}";
    _server->send(200, "application/json", json);
}

void WebInterface::handleClose() {
    _shutter->close();
    String json = "{\"position\":\"CLOSED\",\"angle\":" + String(_shutter->closeAngle()) + "}";
    _server->send(200, "application/json", json);
}

void WebInterface::handleSave() {
    if (_server->hasArg("open") && _server->hasArg("close")) {
        long o = 0;
        long c = 0;
        if (parseIntArg(_server, "open", 0, 180, o) &&
            parseIntArg(_server, "close", 0, 180, c)) {
            _shutter->setOpenAngle((int)o);
            _shutter->setCloseAngle((int)c);
            settings::saveInt(config::NVS_NAMESPACE, config::NVS_KEY_OPEN,  (int)o);
            settings::saveInt(config::NVS_NAMESPACE, config::NVS_KEY_CLOSE, (int)c);
            _server->send(200, "application/json", "{\"ok\":true}");
            return;
        }
    }
    _server->send(400, "application/json", "{\"ok\":false}");
}

void WebInterface::handleSnapshot() {
    if (!_server->hasArg("duration")) {
        _server->send(400, "application/json", "{\"ok\":false,\"error\":\"missing duration\"}");
        return;
    }
    long dur = 0;
    if (!parseIntArg(_server, "duration",
                     (long)config::MIN_SNAPSHOT_MS,
                     (long)config::MAX_SNAPSHOT_MS, dur)) {
        _server->send(400, "application/json", "{\"ok\":false,\"error\":\"invalid duration\"}");
        return;
    }

    _shutter->snapshot((unsigned long)dur);

    if (_autoFeedEnabled) {
        _autoFeedPending = true;
        _autoFeedDelayStarted = false;
    }

    String json = "{\"position\":\"OPEN\",\"angle\":" + String(_shutter->openAngle())
                + ",\"duration\":" + String(dur) + "}";
    _server->send(200, "application/json", json);
}

void WebInterface::handleFeed() {
    int rot = _feeder->feedRotations();
    int speed = _feeder->feedSpeedStepsPerSec();
    if (_server->hasArg("rotations")) {
        long r = 0;
        if (!parseIntArg(_server, "rotations",
                         config::feeder::MIN_FEED_ROTATIONS,
                         config::feeder::MAX_FEED_ROTATIONS, r)) {
            _server->send(400, "application/json", "{\"ok\":false,\"error\":\"invalid rotations\"}");
            return;
        }
        rot = (int)r;
    }
    if (_server->hasArg("speed")) {
        long s = 0;
        if (!parseIntArg(_server, "speed",
                         config::feeder::MIN_FEED_SPEED_STEPS_PER_SEC,
                         config::feeder::MAX_FEED_SPEED_STEPS_PER_SEC, s)) {
            _server->send(400, "application/json", "{\"ok\":false,\"error\":\"invalid speed\"}");
            return;
        }
        speed = (int)s;
    }
    if (_feeder->isFeeding()) {
        _server->send(409, "application/json", "{\"ok\":false,\"error\":\"feeder busy\"}");
        return;
    }
    _feeder->setFeedSpeedStepsPerSec(speed, false);
    _feeder->feed(rot);
    String json = "{\"ok\":true,\"rotations\":" + String(rot)
                + ",\"speed\":" + String(speed) + "}";
    _server->send(200, "application/json", json);
}

void WebInterface::handleFeedJog() {
    if (!_server->hasArg("steps")) {
        _server->send(400, "application/json", "{\"ok\":false,\"error\":\"missing steps\"}");
        return;
    }
    long steps = 0;
    if (!parseIntArg(_server, "steps",
                     -config::feeder::MAX_JOG_STEPS,
                     config::feeder::MAX_JOG_STEPS, steps) ||
        steps == 0) {
        _server->send(400, "application/json", "{\"ok\":false,\"error\":\"invalid steps\"}");
        return;
    }
    if (_feeder->isFeeding()) {
        _server->send(409, "application/json", "{\"ok\":false,\"error\":\"feeder busy\"}");
        return;
    }
    if (_server->hasArg("speed")) {
        long speed = 0;
        if (!parseIntArg(_server, "speed",
                         config::feeder::MIN_FEED_SPEED_STEPS_PER_SEC,
                         config::feeder::MAX_FEED_SPEED_STEPS_PER_SEC, speed)) {
            _server->send(400, "application/json", "{\"ok\":false,\"error\":\"invalid speed\"}");
            return;
        }
        _feeder->setFeedSpeedStepsPerSec((int)speed, false);
    }
    _feeder->jog((int)steps);
    String json = "{\"ok\":true,\"steps\":" + String(steps) + "}";
    _server->send(200, "application/json", json);
}

void WebInterface::handleFeedStop() {
    _feeder->stop();
    _server->send(200, "application/json", "{\"ok\":true}");
}

void WebInterface::handleFeedStatus() {
    String json = "{\"feeding\":" + String(_feeder->isFeeding() ? "true" : "false") + "}";
    _server->send(200, "application/json", json);
}

void WebInterface::handleFeedSettingsGet() {
    String json = "{\"rotations\":" + String(_feeder->feedRotations())
                + ",\"speed\":" + String(_feeder->feedSpeedStepsPerSec())
                + ",\"autoFeed\":" + String(_autoFeedEnabled ? "true" : "false") + "}";
    _server->send(200, "application/json", json);
}

void WebInterface::handleFeedSettingsSave() {
    bool changed = false;
    if (_server->hasArg("rotations")) {
        long rot = 0;
        if (!parseIntArg(_server, "rotations",
                         config::feeder::MIN_FEED_ROTATIONS,
                         config::feeder::MAX_FEED_ROTATIONS, rot)) {
            _server->send(400, "application/json", "{\"ok\":false,\"error\":\"invalid rotations\"}");
            return;
        }
        _feeder->setFeedRotations((int)rot);
        changed = true;
    }
    if (_server->hasArg("speed")) {
        long speed = 0;
        if (!parseIntArg(_server, "speed",
                         config::feeder::MIN_FEED_SPEED_STEPS_PER_SEC,
                         config::feeder::MAX_FEED_SPEED_STEPS_PER_SEC, speed)) {
            _server->send(400, "application/json", "{\"ok\":false,\"error\":\"invalid speed\"}");
            return;
        }
        _feeder->setFeedSpeedStepsPerSec((int)speed);
        changed = true;
    }
    if (changed) {
        _server->send(200, "application/json", "{\"ok\":true}");
        return;
    }
    _server->send(400, "application/json", "{\"ok\":false,\"error\":\"missing setting\"}");
}

void WebInterface::handleFeedAuto() {
    if (_server->hasArg("enabled")) {
        long enabled = 0;
        if (!parseIntArg(_server, "enabled", 0, 1, enabled)) {
            _server->send(400, "application/json", "{\"ok\":false}");
            return;
        }
        _autoFeedEnabled = enabled != 0;
        settings::saveInt(config::feeder::NVS_NAMESPACE,
                          config::feeder::NVS_KEY_AUTO,
                          _autoFeedEnabled ? 1 : 0);
        _server->send(200, "application/json", "{\"ok\":true}");
        return;
    }
    _server->send(400, "application/json", "{\"ok\":false}");
}

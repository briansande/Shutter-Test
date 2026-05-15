#include "web.h"
#include "html.h"
#include "settings.h"
#include "config.h"
#include <WiFi.h>

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
    _server->on("/feed/settings", HTTP_GET,      [this](){ handleFeedSettingsGet(); });
    _server->on("/feed/settings", HTTP_POST,     [this](){ handleFeedSettingsSave(); });
    _server->on("/feed/auto",         HTTP_POST, [this](){ handleFeedAuto(); });

    _server->begin();
    Serial.println("[Web] server started");
}

void WebInterface::loop() {
    _server->handleClient();

    if (_autoFeedPending && !_shutter->isOpen()) {
        _autoFeedPending = false;
        _feeder->feed(_feeder->feedRotations());
    }
}

void WebInterface::handleRoot() {
    size_t sz = sizeof(HTML_PAGE) + 160;
    char* buf = new char[sz];
    snprintf(buf, sz, HTML_PAGE,
             _shutter->openAngle(),
             _shutter->closeAngle(),
             _feeder->feedRotations(),
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
        int o = _server->arg("open").toInt();
        int c = _server->arg("close").toInt();
        if (o >= 0 && o <= 180 && c >= 0 && c <= 180) {
            _shutter->setOpenAngle(o);
            _shutter->setCloseAngle(c);
            settings::saveInt(config::NVS_NAMESPACE, config::NVS_KEY_OPEN,  o);
            settings::saveInt(config::NVS_NAMESPACE, config::NVS_KEY_CLOSE, c);
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
    long dur = _server->arg("duration").toInt();
    if (dur < 1) dur = 500;

    _shutter->snapshot((unsigned long)dur);

    if (_autoFeedEnabled) {
        _autoFeedPending = true;
    }

    String json = "{\"position\":\"OPEN\",\"angle\":" + String(_shutter->openAngle())
                + ",\"duration\":" + String(dur) + "}";
    _server->send(200, "application/json", json);
}

void WebInterface::handleFeed() {
    int rot = _feeder->feedRotations();
    if (_server->hasArg("rotations")) {
        int r = _server->arg("rotations").toInt();
        if (r >= 1) rot = r;
    }
    _feeder->feed(rot);
    String json = "{\"ok\":true,\"rotations\":" + String(rot) + "}";
    _server->send(200, "application/json", json);
}

void WebInterface::handleFeedJog() {
    if (!_server->hasArg("steps")) {
        _server->send(400, "application/json", "{\"ok\":false,\"error\":\"missing steps\"}");
        return;
    }
    int steps = _server->arg("steps").toInt();
    _feeder->jog(steps);
    String json = "{\"ok\":true,\"steps\":" + String(steps) + "}";
    _server->send(200, "application/json", json);
}

void WebInterface::handleFeedStop() {
    _feeder->stop();
    _server->send(200, "application/json", "{\"ok\":true}");
}

void WebInterface::handleFeedSettingsGet() {
    String json = "{\"rotations\":" + String(_feeder->feedRotations())
                + ",\"autoFeed\":" + String(_autoFeedEnabled ? "true" : "false") + "}";
    _server->send(200, "application/json", json);
}

void WebInterface::handleFeedSettingsSave() {
    if (_server->hasArg("rotations")) {
        int rot = _server->arg("rotations").toInt();
        if (rot >= 1) {
            _feeder->setFeedRotations(rot);
            _server->send(200, "application/json", "{\"ok\":true}");
            return;
        }
    }
    _server->send(400, "application/json", "{\"ok\":false}");
}

void WebInterface::handleFeedAuto() {
    if (_server->hasArg("enabled")) {
        _autoFeedEnabled = _server->arg("enabled").toInt() != 0;
        settings::saveInt(config::feeder::NVS_NAMESPACE,
                          config::feeder::NVS_KEY_AUTO,
                          _autoFeedEnabled ? 1 : 0);
        _server->send(200, "application/json", "{\"ok\":true}");
        return;
    }
    _server->send(400, "application/json", "{\"ok\":false}");
}

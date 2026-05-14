#include "web.h"
#include "html.h"
#include "settings.h"
#include "config.h"
#include <WiFi.h>

void WebInterface::begin(Shutter& shutter, uint16_t port) {
    _shutter = &shutter;
    _server  = new WebServer(port);

    _server->on("/",     HTTP_GET,  [this](){ handleRoot(); });
    _server->on("/open", HTTP_POST, [this](){ handleOpen(); });
    _server->on("/close",HTTP_POST, [this](){ handleClose(); });
    _server->on("/save", HTTP_POST, [this](){ handleSave(); });
    _server->on("/snapshot", HTTP_POST, [this](){ handleSnapshot(); });

    _server->begin();
    Serial.println("[Web] server started");
}

void WebInterface::loop() {
    _server->handleClient();
}

void WebInterface::handleRoot() {
    char buf[sizeof(HTML_PAGE) + 96];
    snprintf(buf, sizeof(buf), HTML_PAGE,
             _shutter->openAngle(),
             _shutter->closeAngle(),
             WiFi.localIP().toString().c_str());
    _server->send(200, "text/html", buf);
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

    String json = "{\"position\":\"OPEN\",\"angle\":" + String(_shutter->openAngle())
                + ",\"duration\":" + String(dur) + "}";
    _server->send(200, "application/json", json);
}

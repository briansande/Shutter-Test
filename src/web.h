#pragma once
#include <Arduino.h>
#include <WebServer.h>
#include "shutter.h"

class WebInterface {
public:
    void begin(Shutter& shutter, uint16_t port = 80);
    void loop();

private:
    void handleRoot();
    void handleOpen();
    void handleClose();
    void handleSave();
    void handleSnapshot();

    WebServer* _server   = nullptr;
    Shutter*   _shutter  = nullptr;
};

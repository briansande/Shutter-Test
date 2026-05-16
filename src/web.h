#pragma once
#include <Arduino.h>
#include <WebServer.h>
#include "shutter.h"
#include "paper_feeder.h"

class WebInterface {
public:
    void begin(Shutter& shutter, PaperFeeder& feeder, uint16_t port = 80);
    void loop();

private:
    void handleRoot();
    void handleOpen();
    void handleClose();
    void handleSave();
    void handleSnapshot();

    void handleFeed();
    void handleFeedJog();
    void handleFeedStop();
    void handleFeedStatus();
    void handleFeedSettingsGet();
    void handleFeedSettingsSave();
    void handleFeedAuto();

    WebServer*   _server          = nullptr;
    Shutter*     _shutter         = nullptr;
    PaperFeeder* _feeder          = nullptr;
    bool         _autoFeedEnabled = false;
    bool         _autoFeedPending = false;
    bool         _autoFeedDelayStarted = false;
    unsigned long _autoFeedDelayStartMs = 0;
};

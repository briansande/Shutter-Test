#pragma once
#include <Arduino.h>

class PaperFeeder {
public:
    void begin(int in1, int in2, int in3, int in4,
               int stepsPerRev, int stepDelayMs,
               int defaultRotations);
    void feed(int rotations);
    void feedSteps(int steps);
    void jog(int steps);
    void stop();
    void loop();

    int  feedRotations() const;
    void setFeedRotations(int rot);
    bool isFeeding() const;

private:
    int  _pins[4];
    int  _stepsPerRev    = 4096;
    int  _stepDelayMs    = 2;
    int  _feedRotations  = 1;
    bool _feeding        = false;

    static const uint8_t HALF_STEP_SEQ[8][4];

    void step(int stepIndex);
    void release();
};

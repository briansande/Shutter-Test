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
    int  feedSpeedStepsPerSec() const;
    void setFeedSpeedStepsPerSec(int speed, bool persist = true);
    bool isFeeding() const;

private:
    int  _pins[4];
    int  _stepsPerRev    = 4096;
    uint32_t _stepIntervalUs = 5000;
    int  _startupStepDelayMs = 8;
    uint32_t _startupStepIntervalUs = 8000;
    int  _startupSettleMs = 50;
    int  _rampSteps      = 32;
    uint32_t _activeStepIntervalUs = 5000;
    int  _feedRotations  = 1;
    int  _feedSpeedStepsPerSec = 200;
    bool _feeding        = false;
    bool _settling       = false;
    int  _direction      = 1;
    int  _seqIdx         = 0;
    int32_t _remainingSteps = 0;
    int32_t _completedSteps = 0;
    uint32_t _lastStepAtUs = 0;

    static const uint8_t DRIVE_SEQ[4][4];

    void queueSteps(int32_t steps);
    void advanceSequence(int direction);
    void updateStepDelay();
    void completeMove();
    void step(int stepIndex);
    void release();
};

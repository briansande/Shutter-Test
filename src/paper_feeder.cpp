#include "paper_feeder.h"
#include "settings.h"
#include "config.h"

const uint8_t PaperFeeder::HALF_STEP_SEQ[8][4] = {
    {1, 0, 0, 0},
    {1, 1, 0, 0},
    {0, 1, 0, 0},
    {0, 1, 1, 0},
    {0, 0, 1, 0},
    {0, 0, 1, 1},
    {0, 0, 0, 1},
    {1, 0, 0, 1},
};

void PaperFeeder::begin(int in1, int in2, int in3, int in4,
                         int stepsPerRev, int stepDelayMs,
                         int defaultRotations) {
    _pins[0] = in1;
    _pins[1] = in2;
    _pins[2] = in3;
    _pins[3] = in4;
    _stepsPerRev   = stepsPerRev;
    _stepDelayMs   = stepDelayMs;
    _feedRotations = settings::loadInt(
        config::feeder::NVS_NAMESPACE,
        config::feeder::NVS_KEY_ROT,
        defaultRotations);

    for (int i = 0; i < 4; i++) {
        pinMode(_pins[i], OUTPUT);
        digitalWrite(_pins[i], LOW);
    }

    Serial.printf("[Feeder] begin pins=%d,%d,%d,%d steps/rev=%d rot=%d\n",
                  _pins[0], _pins[1], _pins[2], _pins[3],
                  _stepsPerRev, _feedRotations);
}

void PaperFeeder::feed(int rotations) {
    feedSteps(rotations * _stepsPerRev);
}

void PaperFeeder::feedSteps(int steps) {
    if (steps == 0) return;
    _feeding = true;
    int direction = (steps > 0) ? 1 : -1;
    int totalSteps = abs(steps);

    Serial.printf("[Feeder] feed %d steps (%s)\n", totalSteps,
                  direction > 0 ? "forward" : "reverse");

    int seqIdx = 0;
    for (int s = 0; s < totalSteps; s++) {
        seqIdx = (seqIdx + direction) & 7;
        if (seqIdx < 0) seqIdx += 8;
        step(seqIdx);
        delay(_stepDelayMs);
    }
    release();
    _feeding = false;
    Serial.println("[Feeder] feed complete");
}

void PaperFeeder::jog(int steps) {
    feedSteps(steps);
}

void PaperFeeder::stop() {
    _feeding = false;
    release();
    Serial.println("[Feeder] stopped");
}

void PaperFeeder::loop() {
}

int  PaperFeeder::feedRotations() const { return _feedRotations; }
void PaperFeeder::setFeedRotations(int rot) {
    _feedRotations = rot;
    settings::saveInt(config::feeder::NVS_NAMESPACE,
                      config::feeder::NVS_KEY_ROT, rot);
    Serial.printf("[Feeder] feedRotations set to %d\n", rot);
}
bool PaperFeeder::isFeeding() const { return _feeding; }

void PaperFeeder::step(int stepIndex) {
    for (int i = 0; i < 4; i++) {
        digitalWrite(_pins[i], HALF_STEP_SEQ[stepIndex][i]);
    }
}

void PaperFeeder::release() {
    for (int i = 0; i < 4; i++) {
        digitalWrite(_pins[i], LOW);
    }
}

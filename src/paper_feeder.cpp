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
    if (_feedRotations < config::feeder::MIN_FEED_ROTATIONS) {
        _feedRotations = config::feeder::MIN_FEED_ROTATIONS;
    } else if (_feedRotations > config::feeder::MAX_FEED_ROTATIONS) {
        _feedRotations = config::feeder::MAX_FEED_ROTATIONS;
    }

    for (int i = 0; i < 4; i++) {
        pinMode(_pins[i], OUTPUT);
        digitalWrite(_pins[i], LOW);
    }

    Serial.printf("[Feeder] begin pins=%d,%d,%d,%d steps/rev=%d rot=%d\n",
                  _pins[0], _pins[1], _pins[2], _pins[3],
                  _stepsPerRev, _feedRotations);
}

void PaperFeeder::feed(int rotations) {
    if (rotations <= 0 || _stepsPerRev <= 0) return;
    if (rotations > INT32_MAX / _stepsPerRev) {
        Serial.println("[Feeder] feed request too large");
        return;
    }
    feedSteps(rotations * _stepsPerRev);
}

void PaperFeeder::feedSteps(int steps) {
    queueSteps((int32_t)steps);
}

void PaperFeeder::jog(int steps) {
    feedSteps(steps);
}

void PaperFeeder::stop() {
    _feeding = false;
    _remainingSteps = 0;
    release();
    Serial.println("[Feeder] stopped");
}

void PaperFeeder::loop() {
    if (!_feeding) return;

    unsigned long now = millis();
    if ((unsigned long)(now - _lastStepAt) < (unsigned long)_stepDelayMs) {
        return;
    }
    _lastStepAt = now;

    _seqIdx = (_seqIdx + _direction) & 7;
    step(_seqIdx);

    if (--_remainingSteps <= 0) {
        _feeding = false;
        release();
        Serial.println("[Feeder] feed complete");
    }
}

int  PaperFeeder::feedRotations() const { return _feedRotations; }
void PaperFeeder::setFeedRotations(int rot) {
    if (rot < config::feeder::MIN_FEED_ROTATIONS) {
        rot = config::feeder::MIN_FEED_ROTATIONS;
    } else if (rot > config::feeder::MAX_FEED_ROTATIONS) {
        rot = config::feeder::MAX_FEED_ROTATIONS;
    }
    _feedRotations = rot;
    settings::saveInt(config::feeder::NVS_NAMESPACE,
                      config::feeder::NVS_KEY_ROT, rot);
    Serial.printf("[Feeder] feedRotations set to %d\n", rot);
}
bool PaperFeeder::isFeeding() const { return _feeding; }

void PaperFeeder::queueSteps(int32_t steps) {
    if (steps == 0) return;
    if (steps == INT32_MIN) return;

    _direction = (steps > 0) ? 1 : -1;
    _remainingSteps = (steps > 0) ? steps : -steps;
    _lastStepAt = 0;
    _feeding = true;

    Serial.printf("[Feeder] queued %ld steps (%s)\n", (long)_remainingSteps,
                  _direction > 0 ? "forward" : "reverse");
}

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

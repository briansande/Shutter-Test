#include "paper_feeder.h"
#include "settings.h"
#include "config.h"

const uint8_t PaperFeeder::DRIVE_SEQ[4][4] = {
    // Two-coil full-step order for IN1, IN2, IN3, IN4. This gives the
    // feed roller more holding torque than half-step's single-coil phases.
    {1, 1, 0, 0},
    {0, 1, 1, 0},
    {0, 0, 1, 1},
    {1, 0, 0, 1},
};

namespace {
int clampFeedSpeed(int speed) {
    if (speed < config::feeder::MIN_FEED_SPEED_STEPS_PER_SEC) {
        return config::feeder::MIN_FEED_SPEED_STEPS_PER_SEC;
    }
    if (speed > config::feeder::MAX_FEED_SPEED_STEPS_PER_SEC) {
        return config::feeder::MAX_FEED_SPEED_STEPS_PER_SEC;
    }
    return speed;
}

uint32_t speedToIntervalUs(int speed) {
    speed = clampFeedSpeed(speed);
    return (1000000UL + ((uint32_t)speed / 2UL)) / (uint32_t)speed;
}

int delayToSpeed(int delayMs) {
    if (delayMs < 1) {
        delayMs = 1;
    }
    return clampFeedSpeed((1000 + (delayMs / 2)) / delayMs);
}
}

void PaperFeeder::begin(int in1, int in2, int in3, int in4,
                         int stepsPerRev, int stepDelayMs,
                         int defaultRotations) {
    _pins[0] = in1;
    _pins[1] = in2;
    _pins[2] = in3;
    _pins[3] = in4;
    _stepsPerRev   = stepsPerRev;
    _feedSpeedStepsPerSec = settings::loadInt(
        config::feeder::NVS_NAMESPACE,
        config::feeder::NVS_KEY_SPEED,
        delayToSpeed(stepDelayMs));
    _feedSpeedStepsPerSec = clampFeedSpeed(_feedSpeedStepsPerSec);
    _stepIntervalUs = speedToIntervalUs(_feedSpeedStepsPerSec);
    _startupStepDelayMs = config::feeder::STARTUP_STEP_DELAY_MS;
    _startupStepIntervalUs = (uint32_t)_startupStepDelayMs * 1000UL;
    if (_startupStepIntervalUs < _stepIntervalUs) {
        _startupStepIntervalUs = _stepIntervalUs;
    }
    _startupSettleMs = config::feeder::STARTUP_SETTLE_MS;
    if (_startupSettleMs < 0) {
        _startupSettleMs = 0;
    }
    _rampSteps = config::feeder::RAMP_STEPS;
    if (_rampSteps < 1) {
        _rampSteps = 1;
    }
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

    Serial.printf("[Feeder] begin pins=%d,%d,%d,%d steps/rev=%d rot=%d speed=%d interval=%lu us startup=%lu us settle=%d ramp=%d\n",
                  _pins[0], _pins[1], _pins[2], _pins[3],
                  _stepsPerRev, _feedRotations, _feedSpeedStepsPerSec,
                  (unsigned long)_stepIntervalUs,
                  (unsigned long)_startupStepIntervalUs, _startupSettleMs,
                  _rampSteps);
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
    _settling = false;
    _remainingSteps = 0;
    release();
    Serial.println("[Feeder] stopped");
}

void PaperFeeder::loop() {
    if (!_feeding) return;

    uint32_t now = micros();
    if (_settling) {
        if ((uint32_t)(now - _lastStepAtUs) < (uint32_t)_startupSettleMs * 1000UL) {
            return;
        }
        _settling = false;
        _lastStepAtUs = now;
        return;
    }

    if ((uint32_t)(now - _lastStepAtUs) < _activeStepIntervalUs) {
        return;
    }
    _lastStepAtUs = now;

    advanceSequence(_direction);
    step(_seqIdx);
    _completedSteps++;

    if (--_remainingSteps <= 0) {
        completeMove();
        return;
    }

    updateStepDelay();
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

int PaperFeeder::feedSpeedStepsPerSec() const { return _feedSpeedStepsPerSec; }

void PaperFeeder::setFeedSpeedStepsPerSec(int speed, bool persist) {
    speed = clampFeedSpeed(speed);
    _feedSpeedStepsPerSec = speed;
    _stepIntervalUs = speedToIntervalUs(speed);
    if (_startupStepIntervalUs < _stepIntervalUs) {
        _startupStepIntervalUs = _stepIntervalUs;
    }
    if (persist) {
        settings::saveInt(config::feeder::NVS_NAMESPACE,
                          config::feeder::NVS_KEY_SPEED, speed);
    }
    Serial.printf("[Feeder] feedSpeed set to %d steps/sec (interval=%lu us)\n",
                  speed, (unsigned long)_stepIntervalUs);
}

bool PaperFeeder::isFeeding() const { return _feeding; }

void PaperFeeder::queueSteps(int32_t steps) {
    if (steps == 0) return;
    if (steps == INT32_MIN) return;

    _direction = (steps > 0) ? 1 : -1;
    _remainingSteps = (steps > 0) ? steps : -steps;
    _completedSteps = 0;
    _activeStepIntervalUs = _startupStepIntervalUs;
    _settling = true;
    _feeding = true;

    Serial.printf("[Feeder] queued %ld steps (%s)\n", (long)_remainingSteps,
                  _direction > 0 ? "forward" : "reverse");

    step(_seqIdx);
    _lastStepAtUs = micros();
}

void PaperFeeder::advanceSequence(int direction) {
    _seqIdx += direction;
    if (_seqIdx >= 4) {
        _seqIdx = 0;
    } else if (_seqIdx < 0) {
        _seqIdx = 3;
    }
}

void PaperFeeder::updateStepDelay() {
    if (_activeStepIntervalUs <= _stepIntervalUs) {
        _activeStepIntervalUs = _stepIntervalUs;
        return;
    }

    uint32_t intervalRange = _startupStepIntervalUs - _stepIntervalUs;
    int32_t rampedSteps = _completedSteps;
    if (rampedSteps > _rampSteps) {
        rampedSteps = _rampSteps;
    }

    _activeStepIntervalUs = _startupStepIntervalUs -
        (uint32_t)(((uint64_t)intervalRange * (uint32_t)rampedSteps) / (uint32_t)_rampSteps);
    if (_activeStepIntervalUs < _stepIntervalUs) {
        _activeStepIntervalUs = _stepIntervalUs;
    }
}

void PaperFeeder::completeMove() {
    _feeding = false;
    _settling = false;
    release();
    Serial.println("[Feeder] feed complete");
}

void PaperFeeder::step(int stepIndex) {
    for (int i = 0; i < 4; i++) {
        digitalWrite(_pins[i], DRIVE_SEQ[stepIndex][i]);
    }
}

void PaperFeeder::release() {
    for (int i = 0; i < 4; i++) {
        digitalWrite(_pins[i], LOW);
    }
}

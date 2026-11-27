#pragma once
#include <Arduino.h>
#define U_FLASH 0
#define U_SPIFFS 100
class FakeUpdate {
public:
    bool begin(size_t size, int type) {
        auto& s = Fake::state;
        ++s.beginCalls;
        if (s.updateRunning) return false;
        s.expectedSize = size;
        s.flashType = type;
        s.updateRunning = s.updateBeginOk;
        return s.updateBeginOk;
    }
    size_t write(uint8_t*, size_t size) {
        if (!Fake::state.updateWriteOk) return 0;
        Fake::state.written += size;
        return size;
    }
    bool end() {
        ++Fake::state.endCalls;
        if (Fake::state.updateEndOk) Fake::state.updateRunning = false;
        return Fake::state.updateEndOk;
    }
    void abort() { ++Fake::state.abortCalls; Fake::state.updateRunning = false; }
};
extern FakeUpdate Update;

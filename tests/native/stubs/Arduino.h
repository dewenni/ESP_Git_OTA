#pragma once
#include <algorithm>
#include <cstdint>
#include <cstdio>
#include <cstdlib>
#include <cstring>
#include <string>
#include <vector>

using std::min;
class String : public std::string {
public:
    using std::string::string;
    String() = default;
    String(const std::string& value) : std::string(value) {}
    explicit String(int value) : std::string(std::to_string(value)) {}
};

namespace Fake {
struct State {
    uint32_t now = 0;
    bool beginOk = true;
    int httpCode = 200;
    String json;
    std::string body = std::string(2500, 'x');
    int contentLength = 2500;
    size_t position = 0;
    size_t stopAfter = SIZE_MAX;
    bool disconnected = false;
    bool zeroRead = false;
    bool updateBeginOk = true;
    bool updateWriteOk = true;
    bool updateEndOk = true;
    bool updateRunning = false;
    int beginCalls = 0;
    int abortCalls = 0;
    int endCalls = 0;
    int httpEndCalls = 0;
    int flashType = -1;
    size_t expectedSize = 0;
    size_t written = 0;
    std::string authType;
    std::string auth;
    int caCalls = 0;
    std::string ca;
};
extern State state;
}
inline uint32_t millis() { return Fake::state.now; }
inline void delay(unsigned ms) { Fake::state.now += ms; }

class Stream {
public:
    int available() {
        auto& s = Fake::state;
        size_t end = std::min(s.body.size(), s.stopAfter);
        return static_cast<int>(s.position < end ? end - s.position : 0);
    }
    size_t readBytes(uint8_t* buffer, size_t size) {
        auto& s = Fake::state;
        if (s.zeroRead) return 0;
        size = std::min(size, static_cast<size_t>(available()));
        memcpy(buffer, s.body.data() + s.position, size);
        s.position += size;
        return size;
    }
};

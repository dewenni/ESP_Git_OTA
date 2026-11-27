#pragma once
#include <Arduino.h>
#define HTTP_CODE_OK 200
#define HTTPC_STRICT_FOLLOW_REDIRECTS 1
class HTTPClient {
    Stream stream;
public:
    bool begin(const char*) { return Fake::state.beginOk; }
    bool begin(const char* url, const char* ca) {
        ++Fake::state.caCalls;
        Fake::state.ca = ca;
        return begin(url);
    }
    void setAuthorizationType(const char* type) { Fake::state.authType = type; }
    void setAuthorization(const char* token) { Fake::state.auth = token; }
    void setFollowRedirects(int) {}
    void addHeader(const char*, const char*) {}
    int GET() { return Fake::state.httpCode; }
    String getString() { return Fake::state.json; }
    int getSize() { return Fake::state.contentLength; }
    Stream& getStream() { return stream; }
    bool connected() { return !Fake::state.disconnected || stream.available() > 0; }
    void end() { ++Fake::state.httpEndCalls; }
};

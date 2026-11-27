# Host regression tests

Requirements: Python 3, Clang with AddressSanitizer/UndefinedBehaviorSanitizer,
and an existing ArduinoJson 7 installation. No packages are downloaded.

Run from the library repository, passing the directory containing ArduinoJson.h:

```sh
python3 tests/run_native.py ../ESP32-Jarolift-Controller/.pio/libdeps/esp32/ArduinoJson/src
```

The runner compiles the actual `src/GithubReleaseOTA.cpp` and its public header
with C++11, optimization, warnings as errors and both sanitizers. ArduinoJson is
real; HTTPClient, Update, Arduino String and the clock are small test doubles.
The existing Clang warning about anonymous C++ typedef structs is excluded.
Build outputs are automatically removed from a temporary directory.

Coverage includes empty/error releases, malformed JSON, author/uploader parsing,
borrowed asset lookup, repeated cleanup, noncopyable clients, allocation failures
in the library's explicit malloc calls, Bearer configuration, successful firmware
and SPIFFS writes, content-length bounds, HTTP errors, disconnects, stalled/zero-byte
reads, clock wraparound, and flash begin/write/end failures.

All explicit library allocations are tracked and must be freed, including on
failure. Limits: allocation injection does not cover STL/ArduinoJson allocations.
Additional sanitizer leak detection depends on the host platform. These are software tests, not evidence
of working TLS, redirects, WiFi, flash partitions, boot selection or OTA on a device.
Hardware validation should include a successful update and an interrupted download
followed by a retry. Do not flash devices automatically to run these tests.

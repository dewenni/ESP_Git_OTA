# Changelog

# v0.0.3 — 2026-11-27

First tagged release of the dewenni/ESP_Git_OTA fork.

## Fixes and improvements

- Initialize release, asset and author fields so failed lookups return safe empty results.
- Abort interrupted downloads and downloads stalled for 30 seconds; reject missing or empty Content-Length before writing flash.
- Make repeated cleanup safe, release nested uploader data and prevent copying the owning OTA client.
- Handle malformed JSON, missing asset fields and failed string allocations without returning partial release data.
- Send Bearer authentication correctly for API requests and asset downloads.
- Parse release authors and asset uploaders, and return release tags even without a release title.
- Avoid unnecessary release/asset copies and document borrowed asset lifetimes.
- Correct examples, public API documentation and repository metadata; add host regression tests and local artifact ignore rules.

## Compatibility and limitations

- Existing OTA return-code values and normal method calls are preserved. Download interruptions return OTA_WRITE_ERROR; invalid Content-Length returns OTA_BEGIN_ERROR.
- Read-only release/asset arguments now use const references. Recompile applications against this version.
- GithubReleaseOTA clients can no longer be copied. Release and asset copies still share string pointers; free only the owning object.
- Certificate verification remains optional through setCa(). No new CA or clock requirement is introduced for existing users; without a CA, the server certificate is not verified.

## Validation

- Host regression tests passed with real ArduinoJson, simulated HTTP/flash/time, AddressSanitizer and UndefinedBehaviorSanitizer.
- Library source, Jarolift controller integration and all three examples compiled with the ESP32 toolchain.
- No complete firmware link, hardware OTA, TLS or redirect test was performed. Validate a successful OTA update and an interrupted download followed by a retry on hardware.

## Installation

```ini
lib_deps =
    https://github.com/dewenni/ESP_Git_OTA.git#v0.0.3
```

This is a source-library release. GitHub provides source archives; no device firmware binaries are included.


# Earlier untagged versions

These entries record version values present in the inherited source history;
they were not published as tagged releases of this fork.

## 0.0.2

- Version recorded in library.json by upstream commit 17a6020.
- Subsequent fork changes before v0.0.3 added an OTA progress callback and copied release strings into separately allocated storage.

## 0.0.1

- Initial GitHub OTA library, originally named ESP32_Git_OTA.
- Renamed to ESP_Git_OTA in the inherited source history.

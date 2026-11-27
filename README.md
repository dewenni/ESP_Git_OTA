# ESP Git OTA

This is Over The Air update library for `ESP32` that uses `GitHub` releases as the `firmware` or `SPIFFS` source.

## 📄Table of Contents

- 🏗️ [Installation](#%EF%B8%8Finstallation)
- 📝 [Usage](#usage)
- ➕ [Methods](#methods)
  - 🎉 [init GithubReleaseOTA Object](#init-githubreleaseota-object)
    - 🚨[Token hint](#token-hint)
  - 🔒️ [Setup CA Certificate](#%EF%B8%8Fsetup-ca-certificate)
  - 🏷️ [Get Tag](#%EF%B8%8Fget-tag)
  - 🔖 [Get Release](#get-release-githubrelease-object)
  - 📦️ [Get Asset](#%EF%B8%8Fget-asset-githubreleaseasset-object)
  - ⚡️ [Flash Firmware or SPIFFS](#%EF%B8%8Fflash-firmware-or-spiffs)
  - ♻️ [Free Memory](#%EF%B8%8Ffree-memory)
- 👽️ [Object](#%EF%B8%8Fobject)
  - [GithubRelease](#githubrelease)
  - [GithubReleaseAsset](#githubreleaseasset)
  - [GithubAuthor](#githubauthor)

## 🏗️Installation

- For PlatformIO:  

Add the following line to your `platformio.ini` file:

```ini
lib_deps =
    https://github.com/dewenni/ESP_Git_OTA.git#v0.0.3
```

This pins the tested library version. The upstream project is
[Hsun1031/ESP_Git_OTA](https://github.com/Hsun1031/ESP_Git_OTA).
See [changelog.md](changelog.md) for version history and [changeNew.md](changeNew.md)
for the current release notes.

## 📝Usage

```cpp
#include <Arduino.h>
#include <GithubReleaseOTA.h>

#define ESP_VERSION "v1.0.0"

GithubRelease release;
bool readyForUpdate = false;

// Public repository
GithubReleaseOTA ota("username", "repository");
// Private repository See Token hint https://github.com/Hsun1031/ESP32_Git_OTA?tab=readme-ov-file#token-hint
// GithubReleaseOTA ota("username", "repository", "token");

void setup() {
    Serial.begin(115200);

    // Setup WiFi connection...

    // Optionally enable HTTPS certificate verification with a PEM CA certificate
    // ota.setCa(PEM_CA_CERT);

    // Get the latest release from GitHub
    release = ota.getLatestRelease();

    // Check if the latest release is newer than the current version
    if (release.tag_name != NULL) {
        if (strcmp(release.tag_name, ESP_VERSION) == 0) {
            Serial.println("Already up to date");
            readyForUpdate = false;

            ota.freeRelease(release);
        } else {
            Serial.println("New version available: " + String(release.tag_name));
            readyForUpdate = true;
        }
    } else {
        ota.freeRelease(release);
    }
}

void loop() {
    if (readyForUpdate) {
        int result = ota.flashFirmware(release, "firmware.bin");
        Serial.println("Flash firmware result: " + String(result));

        if (result == 0) {
            Serial.println("Firmware updated successfully");

            ESP.restart();
        } else {
            Serial.println("Firmware update failed: " + String(result));
            ota.freeRelease(release);
            readyForUpdate = false;
        }
    }
}

```

## ➕Methods

### 🎉init GithubReleaseOTA Object

#### ✨`GithubReleaseOTA(const char* username, const char* repository)` Initialize GithubReleaseOTA object with public repository

- `Parameters`:
  - `username` - `const char*`: GitHub username
  - `repository` - `const char*`: GitHub repository

#### ✨`GithubReleaseOTA(const char* username, const char* repository, const char* token)` Initialize GithubReleaseOTA object with private repository

- `Parameters`:
  - `username` - `const char*`: GitHub username
  - `repository` - `const char*`: GitHub repository
  - `token` - `const char*`: GitHub token

#### 🚨`token` hint

Token need `Contents` repository permissions (read), see Github API: [Get a release asset](https://docs.github.com/en/rest/releases/assets?apiVersion=2022-11-28#get-a-release-asset)

### 🔒️Setup CA Certificate

#### ✨`void setCa(const char* ca)` Set website CA (Certificate Authority)

- `Parameters`:
  - `ca` - `const char*`: Certificate Authority

example:

```cpp
const char* PEM_CA_CERT = \
    "-----BEGIN CERTIFICATE-----\n" \
    ...
    ...
    "-----END CERTIFICATE-----\n";

ota.setCa(PEM_CA_CERT);
```

Without `setCa()`, HTTPS connections do **not** verify the server certificate.
Pass a trusted PEM CA certificate to enable verification; the device must also
have a valid clock for certificate validity checks. `setCa(nullptr)` or
`setCa("")` removes a configured CA. A failed CA allocation keeps the previous CA.

### 🏷️Get Tag

#### ✨`String getLatestReleaseTag()` Get Latest release tag

- `Returns`:
  - `String`: Latest release

#### ✨`std::vector<String> getReleaseTagList()` Get all release tags

- `Returns`:
  - `std::vector<String>`: List of release tags

example:

```cpp
std::vector<String> tags = ota.getReleaseTagList();
for (String tag : tags) {
    Serial.println(tag);
}
```

### 🔖Get Release ([`GithubRelease`](#githubrelease) Object)

#### ✨`GithubRelease getLatestRelease()` Get Latest release

- `Returns`:
  - [`GithubRelease`](#githubrelease): Latest release

#### ✨`GithubRelease getReleaseByTagName(const char* tagName)` Get release by tag name

- `Parameters`:
  - `tagName` - `const char*`: Tag name
- `Returns`:
  - [`GithubRelease`](#githubrelease): Release

### 📦️Get Asset ([`GithubReleaseAsset`](#githubreleaseasset) Object)

#### ✨`GithubReleaseAsset getAssetByname(const GithubRelease& release, const char* name)` Get asset by asset name

- `Parameters`:
  - `release` - [`GithubRelease`](#githubrelease): Release
  - `name` - `const char*`: Asset name
- `Returns`:
  - [`GithubReleaseAsset`](#githubreleaseasset): Asset

### ⚡️Flash Firmware or SPIFFS

To Flash Firmware or SPIFFS, Use on `Loop` function

- Returns `int`:
  - `0`: Success
  - `1`: Null URL
  - `2`: Connect error
  - `3`: Begin error
  - `4`: Write error
  - `5`: End error

#### ✨`int flashFirmware(const GithubReleaseAsset& asset);` Flash firmware by asset

- `Parameters`:
  - `asset` - [`GithubReleaseAsset`](#githubreleaseasset): Asset

#### ✨`int flashFirmware(const GithubRelease& release, const char* name)` Flash firmware by release and asset name

- `Parameters`:
  - `release` - [`GithubRelease`](#githubrelease): Release
  - `name` - `const char*`: Asset name

#### ✨`int flashSpiffs(const GithubReleaseAsset& asset)` Flash SPIFFS by asset

- `Parameters`:
  - `asset` - [`GithubReleaseAsset`](#githubreleaseasset): Asset

#### ✨`int flashSpiffs(const GithubRelease& release, const char* name)` Flash SPIFFS by release and asset name

- `Parameters`:
  - `release` - [`GithubRelease`](#githubrelease): Release
  - `name` - `const char*`: Asset name

#### ✨`int GithubReleaseOTA::flashByAssetId(int assetId, int flashType)` Flash by asset id

- `Parameters`:
  - `assetId` - `int`: Asset id
  - `flashType` - `int`: Flash type (`Firmware`: U_FLASH, `SPIFFS`: U_SPIFFS)

Downloads require a positive HTTP Content-Length. Missing/empty lengths return
`OTA_BEGIN_ERROR` before writing flash. A disconnected or stalled download
(30 seconds without received data) is aborted and returns `OTA_WRITE_ERROR`.
Existing return-code values are unchanged. Progress reports received/written
bytes; only `OTA_SUCCESS` confirms final update validation.

### ♻️Free Memory

- ✨`void freeRelease(GithubRelease& release)` Free release object
- ✨`void freeReleaseAsset(GithubReleaseAsset& asset)` Free asset object
- ✨`void freeAuthor(GithubAuthor& author)` Free author object

Release objects own their dynamically allocated strings, including nested author
and uploader data. Call `freeRelease()` once finished; repeated calls on the
**same object** are safe and reset it to an empty state. Standalone owned authors
and assets can be released using their corresponding free functions.

Copies of `GithubRelease`, `GithubReleaseAsset`, and `GithubAuthor` share string
pointers. `getAssetByname()` returns such a **borrowed** asset: keep the original
release alive while using it, and do not call `freeReleaseAsset()` on the borrowed
copy. Free only the owning object, never both an object and its shallow copy.
Do not overwrite an owning release with another result before freeing it.

The `GithubReleaseOTA` client cannot be copied. `clear()` safely releases its
repository URL, token, CA and progress callback; repeated calls and subsequent
destruction are safe. After `clear()`, lookups return empty results and flashing
returns `OTA_NULL_URL`. Construct a new client to use a repository again.

## 👽️Object

See Github API for more information: [GitHub API](https://docs.github.com/en/rest/releases/releases?apiVersion=2022-11-28)

### GithubRelease

- `url`: const char*
- `assets_url`: const char*
- `upload_url`: const char*
- `html_url`: const char*
- `id`: int
- `node_id`: const char*
- `tag_name`: const char*
- `target_commitish`: const char*
- `name`: const char*
- `draft`: bool
- `prerelease`: bool
- `created_at`: const char*
- `published_at`: const char*
- `author`: std::vector\<[GithubAuthor](#githubauthor)\>
- `assets`: std::vector\<[GithubReleaseAsset](#githubreleaseasset)\>

### GithubReleaseAsset

- `url`: const char*
- `browser_download_url`: const char*
- `id`: int
- `node_id`: const char*
- `name`: const char*
- `label`: const char*
- `content_type`: const char*
- `state`: const char*
- `size`: int
- `download_count`: int
- `created_at`: const char*
- `updated_at`: const char*
- `uploader`: std::vector\<[GithubAuthor](#githubauthor)\>

### GithubAuthor

- `login`: const char*
- `id`: int
- `node_id`: const char*
- `avatar_url`: const char*
- `gravatar_id`: const char*
- `url`: const char*
- `html_url`: const char*
- `followers_url`: const char*
- `following_url`: const char*
- `gists_url`: const char*
- `starred_url`: const char*
- `subscriptions_url`: const char*
- `organizations_url`: const char*
- `repos_url`: const char*
- `events_url`: const char*
- `received_events_url`: const char*
- `type`: const char*
- `site_admin`: bool

## Regression tests

See [tests/README.md](tests/README.md) for host tests covering parsing, memory
ownership and OTA error paths. These tests use real ArduinoJson with simulated
HTTP, flash and time; they do not exercise hardware or TLS.

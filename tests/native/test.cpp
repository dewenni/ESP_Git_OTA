#include <GithubReleaseOTA.h>
#include <cassert>
#include <iostream>
#include <set>
#include <type_traits>

Fake::State Fake::state;
FakeUpdate Update;
static int failAfter = -1;
static int allocationCount = 0;
static std::set<void*> allocations;
void* otaTestMalloc(size_t size) {
    ++allocationCount;
    if (failAfter == 0) return nullptr;
    if (failAfter > 0) --failAfter;
    void* pointer = std::malloc(size);
    if (pointer) allocations.insert(pointer);
    return pointer;
}
void otaTestFree(void* pointer) {
    if (pointer) assert(allocations.erase(pointer) == 1);
    std::free(pointer);
}

static const char* releaseJson = R"({
  "tag_name":"v1.2.3", "html_url":"https://example.test/release",
  "author":{"login":"maintainer", "id":1, "site_admin":true},
  "assets":[{"id":42, "name":"firmware.bin", "size":2500,
    "browser_download_url":"https://example.test/firmware.bin",
    "uploader":{"login":"builder", "id":2}},
    {"id":43, "name":"spiffs.bin", "size":2500,
    "browser_download_url":"https://example.test/spiffs.bin"}]
})";

static void reset() {
    Fake::state = {};
    failAfter = -1;
    allocationCount = 0;
}
static void empty(const GithubRelease& release) {
    assert(release.tag_name == nullptr && release.id == 0);
    assert(!release.draft && !release.prerelease);
    assert(release.assets.empty() && release.author.empty());
}
static std::vector<int> progress;
static void onProgress(int value) { progress.push_back(value); }

static void parsingAndOwnership() {
    reset();
    GithubReleaseOTA ota("owner", "repo", "secret");
    GithubRelease initial;
    empty(initial);
    ota.freeRelease(initial);
    ota.freeRelease(initial);
    Fake::state.json = releaseJson;
    auto release = ota.getLatestRelease();
    assert(Fake::state.authType == "Bearer" && Fake::state.auth == "secret");
    assert(strcmp(release.tag_name, "v1.2.3") == 0);
    assert(release.name == nullptr); // A title is not required for tag lookup.
    assert(release.author.size() == 1 && release.author[0].site_admin);
    assert(strcmp(release.author[0].login, "maintainer") == 0);
    assert(release.assets.size() == 2);
    assert(release.assets[0].uploader.size() == 1);
    assert(strcmp(release.assets[0].uploader[0].login, "builder") == 0);
    assert(ota.getLatestReleaseTag() == "v1.2.3");
    auto asset = ota.getAssetByname(release, "firmware.bin");
    assert(asset.id == 42 && asset.size == 2500);
    auto missing = ota.getAssetByname(release, "missing.bin");
    assert(missing.id == 0 && missing.size == 0 && missing.download_count == 0);
    assert(ota.getAssetByname(release, nullptr).name == nullptr);
    GithubRelease malformed;
    malformed.assets.push_back(GithubReleaseAsset{});
    assert(ota.getAssetByname(malformed, "firmware.bin").name == nullptr);
    ota.freeRelease(malformed);
    ota.freeAuthor(release.author[0]);
    ota.freeAuthor(release.author[0]);
    assert(release.author[0].login == nullptr && !release.author[0].site_admin);
    ota.freeReleaseAsset(release.assets[0]);
    ota.freeReleaseAsset(release.assets[0]);
    assert(release.assets[0].name == nullptr && release.assets[0].uploader.empty());
    ota.freeRelease(release);
    empty(release);
    ota.freeRelease(release);
    // The borrowed asset is deliberately not freed or used after its owner.

    for (const char* json : {"{", "[]", "{}", "{\"tag_name\":null,\"assets\":[]}",
                            "{\"tag_name\":\"v1\",\"assets\":[{}]}",
                            "{\"tag_name\":\"v1\",\"assets\":[{\"id\":1,\"name\":null}]}"}) {
        Fake::state.json = json;
        auto failed = ota.getLatestRelease();
        empty(failed);
        ota.freeRelease(failed);
    }
    Fake::state.httpCode = 500;
    auto failed = ota.getLatestRelease();
    empty(failed);
    ota.freeRelease(failed);
    failed = ota.getReleaseByTagName("v1");
    empty(failed);
    ota.freeRelease(failed);
    failed = ota.getReleaseByTagName(nullptr);
    empty(failed);
    Fake::state.httpCode = 200;
    Fake::state.beginOk = false;
    failed = ota.getLatestRelease();
    empty(failed);
    ota.freeRelease(failed);

    reset();
    Fake::state.json = R"([{"tag_name":"v2"},{"tag_name":null},{"tag_name":"v1"}])";
    auto tags = ota.getReleaseTagList();
    assert(tags.size() == 2 && tags[0] == "v2" && tags[1] == "v1");
    Fake::state.json = "[";
    assert(ota.getReleaseTagList().empty());
    Fake::state.json = "{}";
    assert(ota.getReleaseTagList().empty());
}

static void allocationFailuresAndClear() {
    reset();
    GithubReleaseOTA ota("owner", "repo");
    Fake::state.json = releaseJson;
    allocationCount = 0;
    auto release = ota.getLatestRelease();
    int allocations = allocationCount; // URL plus all release/author/uploader strings.
    assert(allocations > 5);
    ota.freeRelease(release);
    for (int i = 0; i < allocations; ++i) {
        failAfter = i;
        auto failed = ota.getLatestRelease();
        empty(failed);
        ota.freeRelease(failed);
        failAfter = -1;
    }
    ota.setCa("old-ca");
    failAfter = 0;
    ota.setCa("new-ca");
    failAfter = -1;
    Fake::state.caCalls = 0;
    release = ota.getLatestRelease();
    assert(Fake::state.caCalls == 1 && Fake::state.ca == "old-ca");
    ota.freeRelease(release);
    ota.setCa(nullptr);
    ota.setCa("");
    ota.clear();
    ota.clear();
    empty(ota.getLatestRelease());
    empty(ota.getReleaseByTagName("v1"));
    assert(ota.getReleaseTagList().empty());
    assert(ota.flashByAssetId(42, U_FLASH) == OTA_NULL_URL);
    // Destructor after clear must also be safe.
    failAfter = 0;
    GithubReleaseOTA noMemory("owner", "repo");
    failAfter = -1;
    empty(noMemory.getLatestRelease());
    GithubReleaseOTA invalid(nullptr, "repo");
    empty(invalid.getLatestRelease());
}

static void downloads() {
    reset();
    GithubReleaseOTA ota("owner", "repo", "secret");
    ota.setProgressCallback(onProgress);
    progress.clear();
    assert(ota.flashByAssetId(42, U_FLASH) == OTA_SUCCESS);
    assert(Fake::state.written == 2500 && Fake::state.expectedSize == 2500);
    assert(Fake::state.endCalls == 1 && Fake::state.abortCalls == 0);
    assert(Fake::state.authType == "Bearer" && Fake::state.auth == "secret");
    assert(!progress.empty() && progress.back() == 100);
    assert(std::is_sorted(progress.begin(), progress.end()));

    reset();
    Fake::state.body.append(100, 'y'); // Never write beyond declared Content-Length.
    assert(ota.flashByAssetId(42, U_SPIFFS) == OTA_SUCCESS);
    assert(Fake::state.written == 2500 && Fake::state.position == 2500);
    assert(Fake::state.flashType == U_SPIFFS);

    for (int size : {-1, 0}) {
        reset();
        Fake::state.contentLength = size;
        assert(ota.flashByAssetId(42, U_FLASH) == OTA_BEGIN_ERROR);
        assert(Fake::state.beginCalls == 0);
    }
    reset();
    Fake::state.stopAfter = 1234;
    Fake::state.disconnected = true;
    assert(ota.flashByAssetId(42, U_FLASH) == OTA_WRITE_ERROR);
    assert(Fake::state.written == 1234 && Fake::state.abortCalls == 1);
    assert(Fake::state.endCalls == 0 && Fake::state.httpEndCalls == 1);
    Fake::state.disconnected = false;
    Fake::state.stopAfter = SIZE_MAX;
    Fake::state.position = 0;
    Fake::state.written = 0;
    assert(ota.flashByAssetId(42, U_FLASH) == OTA_SUCCESS);

    for (bool zeroRead : {false, true}) {
        reset();
        Fake::state.now = UINT32_MAX - 100; // Timeout must survive millis() wraparound.
        Fake::state.stopAfter = zeroRead ? SIZE_MAX : 1234;
        Fake::state.zeroRead = zeroRead;
        assert(ota.flashByAssetId(42, U_FLASH) == OTA_WRITE_ERROR);
        assert(Fake::state.abortCalls == 1 && Fake::state.now < 31000);
    }
    reset();
    Fake::state.updateWriteOk = false;
    assert(ota.flashByAssetId(42, U_FLASH) == OTA_WRITE_ERROR);
    assert(Fake::state.abortCalls == 1);
    reset();
    Fake::state.updateEndOk = false;
    assert(ota.flashByAssetId(42, U_FLASH) == OTA_END_ERROR);
    assert(Fake::state.abortCalls == 1);
    reset();
    Fake::state.updateBeginOk = false;
    assert(ota.flashByAssetId(42, U_FLASH) == OTA_BEGIN_ERROR);
    reset();
    Fake::state.httpCode = 404;
    assert(ota.flashByAssetId(42, U_FLASH) == OTA_CONNECT_ERROR);
    assert(Fake::state.beginCalls == 0);
    reset();
    Fake::state.beginOk = false;
    assert(ota.flashByAssetId(42, U_FLASH) == OTA_CONNECT_ERROR);
    reset();
    assert(ota.flashByAssetId(0, U_FLASH) == OTA_NULL_URL);
    GithubReleaseAsset absent;
    assert(ota.flashFirmware(absent) == OTA_NULL_URL);
    assert(ota.flashSpiffs(absent) == OTA_NULL_URL);
    GithubRelease release;
    assert(ota.flashFirmware(release, nullptr) == OTA_NULL_URL);
    assert(ota.flashSpiffs(release, "missing") == OTA_NULL_URL);
    reset();
    Fake::state.json = releaseJson;
    release = ota.getReleaseByTagName("v1.2.3");
    assert(release.assets.size() == 2);
    assert(ota.flashFirmware(release, "firmware.bin") == OTA_SUCCESS);
    Fake::state.position = 0;
    assert(ota.flashSpiffs(release, "spiffs.bin") == OTA_SUCCESS);
    ota.freeRelease(release);
}

int main() {
    static_assert(!std::is_copy_constructible<GithubReleaseOTA>::value, "client must not copy owning pointers");
    static_assert(!std::is_copy_assignable<GithubReleaseOTA>::value, "client must not copy owning pointers");
    parsingAndOwnership();
    assert(allocations.empty());
    allocationFailuresAndClear();
    assert(allocations.empty());
    downloads();
    assert(allocations.empty());
    std::cout << "PASS: parsing, ownership, allocation failures, authentication, OTA success and error paths\n";
}

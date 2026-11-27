#ifndef __GITHUB_RELEASE_OTA_H__
#define __GITHUB_RELEASE_OTA_H__
    #include <Arduino.h>

    #include <HTTPClient.h>
    #include <Update.h>
    #include <ArduinoJson.h>

    #include <vector>

    #include <esp_log.h>

    #define GITHUB_API_RELEASE_URL        "https://api.github.com/repos/%s/%s/releases"

    #define GITHUB_API_LATEST_RELEASE_URL "%s/latest"
    #define GITHUB_API_TAGS_RELEASE_URL   "%s/tags/%s"
    #define GITHUB_API_RELEASE_ASSETS_URL "%s/assets/%s"

    #define GITHUB_API_RELEASE_ASSETS_ACCEPT_JSON         "application/vnd.github+json"
    #define GITHUB_API_RELEASE_ASSETS_ACCEPT_OCTET_STREAM "application/octet-stream"

    #define X_GITHUB_API_VERSION "2022-11-28"

    #define OTA_SUCCESS       0
    #define OTA_NULL_URL      1
    #define OTA_CONNECT_ERROR 2
    #define OTA_BEGIN_ERROR   3
    #define OTA_WRITE_ERROR   4
    #define OTA_END_ERROR     5

    #define FLASH_TYPE_FIRMWARE U_FLASH
    #define FLASH_TYPE_SPIFFS   U_SPIFFS

    #define GITHUB_OTA_FIRMWARE_NAME "firmware.bin"
    #define GITHUB_OTA_SPIFFS_NAME "spiffs.bin"

    typedef struct {
        const char* login = NULL;
        int id = 0;
        const char* node_id = NULL;
        const char* avatar_url = NULL;
        const char* gravatar_id = NULL;
        const char* url = NULL;
        const char* html_url = NULL;
        const char* followers_url = NULL;
        const char* following_url = NULL;
        const char* gists_url = NULL;
        const char* starred_url = NULL;
        const char* subscriptions_url = NULL;
        const char* organizations_url = NULL;
        const char* repos_url = NULL;
        const char* events_url = NULL;
        const char* received_events_url = NULL;
        const char* type = NULL;
        bool site_admin = false;
    } GithubAuthor;

    typedef struct {
        const char* url = NULL;
        const char* browser_download_url = NULL;
        int id = 0;
        const char* node_id = NULL;
        const char* name = NULL;
        const char* label = NULL;
        const char* state = NULL;
        const char* content_type = NULL;
        int size = 0;
        int download_count = 0;
        const char* created_at = NULL;
        const char* updated_at = NULL;
        std::vector<GithubAuthor> uploader;
    } GithubReleaseAsset;

    typedef struct {
        const char* url = NULL;
        const char* html_url = NULL;
        const char* assets_url = NULL;
        const char* upload_url = NULL;
        const char* tarball_url = NULL;
        const char* zipball_url = NULL;
        int id = 0;
        const char* node_id = NULL;
        const char* tag_name = NULL;
        const char* target_commitish = NULL;
        const char* name = NULL;
        const char* body = NULL;
        bool draft = false;
        bool prerelease = false;
        const char* created_at = NULL;
        const char* published_at = NULL;
        std::vector<GithubReleaseAsset> assets;
        std::vector<GithubAuthor> author;
    } GithubRelease;

    class GithubReleaseOTA {
        private:
            char* releaseUrl = NULL;
            char* token = NULL;
            char* ca =  NULL;
            void (*progressCallback)(int) = nullptr;

        public:
            GithubReleaseOTA(const char* owner, const char* repo, const char* token = (const char*)NULL);
            ~GithubReleaseOTA();

            GithubReleaseOTA(const GithubReleaseOTA&) = delete;
            GithubReleaseOTA& operator=(const GithubReleaseOTA&) = delete;

            // Release client configuration; safe to call again before destruction.
            void clear();

            void setCa(const char* ca);

            String getLatestReleaseTag();
            std::vector<String> getReleaseTagList();

            GithubRelease getLatestRelease();

            GithubRelease getReleaseByTagName(const char* tagName);

            // Borrowed result: keep release alive and free only the owning release.
            GithubReleaseAsset getAssetByname(const GithubRelease& release, const char* name);

            int flashFirmware(const GithubReleaseAsset& asset);
            int flashFirmware(const GithubRelease& release, const char* name = GITHUB_OTA_FIRMWARE_NAME);

            int flashSpiffs(const GithubReleaseAsset& asset);
            int flashSpiffs(const GithubRelease& release, const char* name = GITHUB_OTA_SPIFFS_NAME);

            int flashByAssetId(int assetId, int flashType);

            // Reset owned data after freeing it. Shallow copies must not be freed separately.
            void freeRelease(GithubRelease& release);
            void freeAuthor(GithubAuthor& author);
            void freeReleaseAsset(GithubReleaseAsset& asset);

            void setProgressCallback(void (*callback)(int)) { this->progressCallback = callback; }

        private:
            String connectGithub(const char* url, int *code);
            GithubRelease makeRelease(const String& payload);
    };

#endif // __GITHUB_RELEASE_OTA_H__

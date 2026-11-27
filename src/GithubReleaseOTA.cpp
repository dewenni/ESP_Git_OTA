#include <GithubReleaseOTA.h>
#include <utility>

/**
 * @brief Construct a new Github Release OTA object
 *
 * @param owner `const char*` Github repository owner
 * @param repo `const char*` Github repository
 * @param token `const char*` Github token
 */
GithubReleaseOTA::GithubReleaseOTA(const char* owner, const char* repo, const char* token) {
    if (owner == NULL || repo == NULL) return;

    int urlSize = snprintf(NULL, 0, GITHUB_API_RELEASE_URL, owner, repo) + 1;
    this->releaseUrl = (char*)malloc(urlSize);
    if (this->releaseUrl != NULL)
        snprintf(this->releaseUrl, urlSize, GITHUB_API_RELEASE_URL, owner, repo);
    else
        ESP_LOGE("GithubReleaseOTA", "Failed to allocate memory for release URL");

    if (token != NULL) {
        this->token = (char*)malloc(strlen(token) + 1);

        if (this->token != NULL)
            strcpy(this->token, token);
        else
            ESP_LOGE("GithubReleaseOTA", "Failed to allocate memory for release Token");
    } else {
        this->token = NULL;
    }
}

/**
 * @brief Destroy the Github Release OTA object
 *
 */
GithubReleaseOTA::~GithubReleaseOTA() { clear(); }

/**
 * @brief Release client resources. Repeated calls are safe.
 */
void GithubReleaseOTA::clear() {
    free(this->releaseUrl);
    free(this->token);
    free(this->ca);
    this->releaseUrl = NULL;
    this->token = NULL;
    this->ca = NULL;
    this->progressCallback = nullptr;
}

/**
 * @brief Set website CA (Certificate Authority)
 *
 * @param ca `const char*` Certificate Authority
 */
void GithubReleaseOTA::setCa(const char* ca) {
    char* copy = NULL;
    if (ca != NULL && ca[0] != '\0') {
        copy = (char*)malloc(strlen(ca) + 1);
        if (copy == NULL) {
            ESP_LOGE("GithubReleaseOTA", "Failed to allocate memory for CA");
            return;
        }
        strcpy(copy, ca);
    }
    free(this->ca);
    this->ca = copy;
}

/**
 * @brief Get Latest release tag
 *
 * @return `String` Latest release tag
 */
String GithubReleaseOTA::getLatestReleaseTag() {
    GithubRelease release = getLatestRelease();
    String tag = "";
    if (release.tag_name != NULL) tag = release.tag_name;
    freeRelease(release);
    return tag;
}

/**
 * @brief Get all release tags
 *
 * @return `std::vector<String>` List of release tags
 */
std::vector<String> GithubReleaseOTA::getReleaseTagList() {
    std::vector<String> tags;

    if (releaseUrl != NULL) {
        int code = 0;
        String payload = connectGithub(this->releaseUrl, &code);
        if (code == HTTP_CODE_OK) {
            JsonDocument releasesList;
            if (deserializeJson(releasesList, payload.c_str()) || !releasesList.is<JsonArray>()) {
                ESP_LOGE("GithubReleaseOTA", "Invalid release list JSON");
                return tags;
            }

            JsonArray releases = releasesList.as<JsonArray>();

            for (JsonObjectConst release : releases) {
                const char* tag = release["tag_name"];
                if (tag != NULL && tag[0] != '\0') tags.push_back(String(tag));
            }

            releasesList.clear();
        }
    }

    return tags;
}

/**
 * @brief Get latest release
 *
 * @return `GithubRelease` Github Release Object
 */
GithubRelease GithubReleaseOTA::getLatestRelease() {
    GithubRelease release{};
    if (this->releaseUrl == NULL) return release;

    int urlSize = snprintf(NULL, 0, GITHUB_API_LATEST_RELEASE_URL, this->releaseUrl) + 1;
    char* url = (char*)malloc(urlSize);

    if (url != NULL) {
        snprintf(url, urlSize, GITHUB_API_LATEST_RELEASE_URL, this->releaseUrl);
        int code = 0;
        String payload = connectGithub(url, &code);
        free(url);

        if (code == HTTP_CODE_OK) {
            release = makeRelease(payload);
        }
    }

    return release;
}

/**
 * @brief Get release by tag name
 *
 * @param tag `const char*` Tag Name
 * @return `GithubRelease` Github Release Object
 */
GithubRelease GithubReleaseOTA::getReleaseByTagName(const char* tagName) {
    GithubRelease release{};

    if (this->releaseUrl == NULL || tagName == NULL || tagName[0] == '\0') {
        return release;
    }

    int urlSize = snprintf(NULL, 0, GITHUB_API_TAGS_RELEASE_URL, this->releaseUrl, tagName) + 1;
    char* url = (char*)malloc(urlSize);

    if (url != NULL) {
        snprintf(url, urlSize, GITHUB_API_TAGS_RELEASE_URL, this->releaseUrl, tagName);
        int code = 0;
        String payload = connectGithub(url, &code);
        free(url);
        if (code == HTTP_CODE_OK) release = makeRelease(payload);
    }

    return release;
}

/**
 * @brief Get asset by name
 *
 * @param release `GithubRelease` Github Release Object
 * @param name `const char*` Asset Name
 * @return `GithubReleaseAsset` Github Release Asset Object
 */
GithubReleaseAsset GithubReleaseOTA::getAssetByname(const GithubRelease& release, const char* name) {
    GithubReleaseAsset asset{};

    if (name == NULL) return asset;

    for (const auto& a : release.assets) {
        if (a.name != NULL && strcmp(a.name, name) == 0) {
            return a;
        }
    }

    return asset;
}

/**
 * @brief Flash firmware
 *
 * @param asset `GithubReleaseAsset` Github Release Asset Object
 * @return `int` OTA Status, `OTA_SUCCESS`:0, `OTA_NULL_URL`:1, `OTA_CONNECT_ERROR`:2, `OTA_BEGIN_ERROR`:3, `OTA_WRITE_ERROR`:4, `OTA_END_ERROR`:5
 */
int GithubReleaseOTA::flashFirmware(const GithubReleaseAsset& asset) { return GithubReleaseOTA::flashByAssetId(asset.id, FLASH_TYPE_FIRMWARE); }

/**
 * @brief Flash firmware
 *
 * @param release `GithubRelease` Github Release Object
 * @param name `const char*` Asset Name
 * @return `int` OTA Status, `OTA_SUCCESS`:0, `OTA_NULL_URL`:1, `OTA_CONNECT_ERROR`:2, `OTA_BEGIN_ERROR`:3, `OTA_WRITE_ERROR`:4, `OTA_END_ERROR`:5
 */
int GithubReleaseOTA::flashFirmware(const GithubRelease& release, const char* name) {
    GithubReleaseAsset asset = getAssetByname(release, name);
    if (asset.browser_download_url == NULL) return OTA_NULL_URL;

    return GithubReleaseOTA::flashByAssetId(asset.id, FLASH_TYPE_FIRMWARE);
}

/**
 * @brief Flash SPIFFS
 *
 * @param asset `GithubReleaseAsset` Github Release Asset Object
 * @return `int` OTA Status, `OTA_SUCCESS`:0, `OTA_NULL_URL`:1, `OTA_CONNECT_ERROR`:2, `OTA_BEGIN_ERROR`:3, `OTA_WRITE_ERROR`:4, `OTA_END_ERROR`:5
 */
int GithubReleaseOTA::flashSpiffs(const GithubReleaseAsset& asset) { return GithubReleaseOTA::flashByAssetId(asset.id, FLASH_TYPE_SPIFFS); }

/**
 * @brief Flash SPIFFS
 *
 * @param release `GithubRelease` Github Release Object
 * @param name `const char*` Asset Name
 * @return `int` OTA Status, `OTA_SUCCESS`:0, `OTA_NULL_URL`:1, `OTA_CONNECT_ERROR`:2, `OTA_BEGIN_ERROR`:3, `OTA_WRITE_ERROR`:4, `OTA_END_ERROR`:5
 */
int GithubReleaseOTA::flashSpiffs(const GithubRelease& release, const char* name) {
    GithubReleaseAsset asset = getAssetByname(release, name);
    if (asset.browser_download_url == NULL) return OTA_NULL_URL;

    return GithubReleaseOTA::flashByAssetId(asset.id, FLASH_TYPE_SPIFFS);
}

/**
 * @brief Flash by asset ID
 *
 * @param assetId `int` Asset ID
 * @param flashType `int` Flash Type, `U_FLASH` or `U_SPIFFS`
 * @return `int` OTA Status, `OTA_SUCCESS`:0, `OTA_NULL_URL`:1, `OTA_CONNECT_ERROR`:2, `OTA_BEGIN_ERROR`:3, `OTA_WRITE_ERROR`:4, `OTA_END_ERROR`:5
 */
int GithubReleaseOTA::flashByAssetId(int assetId, int flashType) {
    if (this->releaseUrl == NULL || assetId <= 0) return OTA_NULL_URL;

    int urlSize = snprintf(NULL, 0, GITHUB_API_RELEASE_ASSETS_URL, this->releaseUrl, String(assetId).c_str()) + 1;
    char* url = (char*)malloc(urlSize);
    if (url == NULL) {
        ESP_LOGE("GithubReleaseOTA", "Failed to allocate memory for asset URL");
        return OTA_NULL_URL;
    }

    snprintf(url, urlSize, GITHUB_API_RELEASE_ASSETS_URL, this->releaseUrl, String(assetId).c_str());

    HTTPClient client;
    if (!(this->ca != NULL ? client.begin(url, this->ca) : client.begin(url))) {
        free(url);
        return OTA_CONNECT_ERROR;
    }

    if (token != NULL) {
        client.setAuthorizationType("Bearer");
        client.setAuthorization(token);
    }
    client.setFollowRedirects(HTTPC_STRICT_FOLLOW_REDIRECTS);
    client.addHeader("Accept", GITHUB_API_RELEASE_ASSETS_ACCEPT_OCTET_STREAM);
    client.addHeader("X-GitHub-Api-Version", X_GITHUB_API_VERSION);

    if (client.GET() != HTTP_CODE_OK) {
        ESP_LOGE("GithubReleaseOTA", "Failed to connect to GitHub API");
        client.end();
        free(url);
        return OTA_CONNECT_ERROR;
    }

    int size = client.getSize();
    if (size <= 0 || !Update.begin(static_cast<size_t>(size), flashType)) {
        ESP_LOGE("GithubReleaseOTA", "Failed to begin OTA update");
        client.end();
        free(url);
        return OTA_BEGIN_ERROR;
    }

    // Use the HTTPClient's stream directly
    Stream& stream = client.getStream();
    size_t written = 0;
    const size_t chunkSize = 1024;  // Set chunk size
    uint8_t buffer[chunkSize];
    int lastProgress = -1;
    uint32_t lastDataAt = millis();
    const uint32_t idleTimeoutMs = 30000;
    const size_t expectedSize = static_cast<size_t>(size);

    while (written < expectedSize) {
        int available = stream.available();
        if ((!client.connected() && available <= 0) ||
            static_cast<uint32_t>(millis() - lastDataAt) >= idleTimeoutMs) {
            ESP_LOGE("GithubReleaseOTA", "OTA download interrupted or timed out");
            Update.abort();
            client.end();
            free(url);
            return OTA_WRITE_ERROR;
        }
        if (available > 0) {
            const size_t readLimit = min(min(static_cast<size_t>(available), chunkSize), expectedSize - written);
            size_t readSize = stream.readBytes(buffer, readLimit);
            if (readSize > 0) {
                if (Update.write(buffer, readSize) != readSize) {
                    ESP_LOGE("GithubReleaseOTA", "Error writing chunk");
                    Update.abort();
                    client.end();
                    free(url);
                    return OTA_WRITE_ERROR;
                }
                written += readSize;
                lastDataAt = millis();
                ESP_LOGD("GithubReleaseOTA", "Written %u/%d bytes", static_cast<unsigned>(written), size);
            }

            int progress = static_cast<int>((static_cast<uint64_t>(written) * 100) / expectedSize);
            if (progress != lastProgress) {
                if (this->progressCallback) {
                    this->progressCallback(progress);
                }
                lastProgress = progress;
            }
        }
        delay(1);
    }

    if (!Update.end()) {
        ESP_LOGE("GithubReleaseOTA", "Failed to end OTA update");
        Update.abort();
        client.end();
        free(url);
        return OTA_END_ERROR;
    }

    ESP_LOGI("GithubReleaseOTA", "OTA update successful");
    client.end();
    free(url);
    return OTA_SUCCESS;
}

/**
 * @brief Free release
 *
 * @param release `GithubRelease&` Github Release Object
 */
void GithubReleaseOTA::freeRelease(GithubRelease& release) {
    if (release.url != NULL) free((void*)release.url);
    if (release.html_url != NULL) free((void*)release.html_url);
    if (release.assets_url != NULL) free((void*)release.assets_url);
    if (release.upload_url != NULL) free((void*)release.upload_url);
    if (release.tarball_url != NULL) free((void*)release.tarball_url);
    if (release.zipball_url != NULL) free((void*)release.zipball_url);
    if (release.node_id != NULL) free((void*)release.node_id);
    if (release.tag_name != NULL) free((void*)release.tag_name);
    if (release.target_commitish != NULL) free((void*)release.target_commitish);
    if (release.name != NULL) free((void*)release.name);
    if (release.body != NULL) free((void*)release.body);
    if (release.created_at != NULL) free((void*)release.created_at);
    if (release.published_at != NULL) free((void*)release.published_at);

    for (auto& author : release.author) {
        freeAuthor(author);
    }

    for (auto& asset : release.assets) {
        freeReleaseAsset(asset);
    }
    release = {};
}

/**
 * @brief Free release asset
 *
 * @param asset `GithubReleaseAsset&` Github Release Asset Object
 */
void GithubReleaseOTA::freeReleaseAsset(GithubReleaseAsset& asset) {
    if (asset.url != NULL) free((void*)asset.url);
    if (asset.node_id != NULL) free((void*)asset.node_id);
    if (asset.name != NULL) free((void*)asset.name);
    if (asset.label != NULL) free((void*)asset.label);
    if (asset.content_type != NULL) free((void*)asset.content_type);
    if (asset.state != NULL) free((void*)asset.state);
    if (asset.created_at != NULL) free((void*)asset.created_at);
    if (asset.updated_at != NULL) free((void*)asset.updated_at);
    if (asset.browser_download_url != NULL) free((void*)asset.browser_download_url);
    for (auto& uploader : asset.uploader) freeAuthor(uploader);
    asset = {};
}

/**
 * @brief Free author
 *
 * @param author `GithubAuthor&` Github Author Object
 */
void GithubReleaseOTA::freeAuthor(GithubAuthor& author) {
    if (author.login != NULL) free((void*)author.login);
    if (author.node_id != NULL) free((void*)author.node_id);
    if (author.avatar_url != NULL) free((void*)author.avatar_url);
    if (author.gravatar_id != NULL) free((void*)author.gravatar_id);
    if (author.url != NULL) free((void*)author.url);
    if (author.html_url != NULL) free((void*)author.html_url);
    if (author.followers_url != NULL) free((void*)author.followers_url);
    if (author.following_url != NULL) free((void*)author.following_url);
    if (author.gists_url != NULL) free((void*)author.gists_url);
    if (author.starred_url != NULL) free((void*)author.starred_url);
    if (author.subscriptions_url != NULL) free((void*)author.subscriptions_url);
    if (author.organizations_url != NULL) free((void*)author.organizations_url);
    if (author.repos_url != NULL) free((void*)author.repos_url);
    if (author.events_url != NULL) free((void*)author.events_url);
    if (author.received_events_url != NULL) free((void*)author.received_events_url);
    if (author.type != NULL) free((void*)author.type);
    author = {};
}

/**
 * @brief Connect to Github Release API
 *
 * @param url `const char*` Github Repo URL
 * @param code `int` HTTP code
 * @return `String` payload
 */
String GithubReleaseOTA::connectGithub(const char* url, int* code) {
    HTTPClient http;
    *code = 0;
    if (url == NULL || !(this->ca != NULL ? http.begin(url, this->ca) : http.begin(url))) return String();
    http.addHeader("Accept", GITHUB_API_RELEASE_ASSETS_ACCEPT_JSON);
    http.addHeader("X-GitHub-Api-Version", X_GITHUB_API_VERSION);
    if (this->token != NULL) {
        http.setAuthorizationType("Bearer");
        http.setAuthorization(this->token);
    }

    *code = http.GET();
    String payload = *code == HTTP_CODE_OK ? http.getString() : String();

    http.end();

    return payload;
}

/**
 * @brief Make release object
 *
 * @param payload `String` Github Release JSON payload
 * @return `GithubRelease` Github Release Object
 */
GithubRelease GithubReleaseOTA::makeRelease(const String& payload) {
    GithubRelease githubRelease{};
    JsonDocument releases;
    if (deserializeJson(releases, payload.c_str()) || !releases.is<JsonObject>() ||
        !releases["tag_name"].is<const char*>() ||
        releases["tag_name"].as<const char*>()[0] == '\0' ||
        !releases["assets"].is<JsonArray>()) {
        ESP_LOGE("GithubReleaseOTA", "Invalid release JSON");
        return githubRelease;
    }

    bool allocationFailed = false;
    auto copyString = [&allocationFailed](const char* str) -> char* {
        if (str == nullptr || allocationFailed) return nullptr;
        char* copy = (char*)malloc(strlen(str) + 1);
        if (copy != nullptr) {
            strcpy(copy, str);
        } else {
            allocationFailed = true;
            ESP_LOGE("GithubReleaseOTA", "Failed to allocate memory for string");
        }
        return copy;
    };

    githubRelease.url = copyString(releases["url"]);
    githubRelease.html_url = copyString(releases["html_url"]);
    githubRelease.assets_url = copyString(releases["assets_url"]);
    githubRelease.upload_url = copyString(releases["upload_url"]);
    githubRelease.tarball_url = copyString(releases["tarball_url"]);
    githubRelease.zipball_url = copyString(releases["zipball_url"]);
    githubRelease.id = releases["id"];
    githubRelease.node_id = copyString(releases["node_id"]);
    githubRelease.tag_name = copyString(releases["tag_name"]);
    githubRelease.target_commitish = copyString(releases["target_commitish"]);
    githubRelease.name = copyString(releases["name"]);
    githubRelease.body = copyString(releases["body"]);
    githubRelease.draft = releases["draft"];
    githubRelease.prerelease = releases["prerelease"];
    githubRelease.created_at = copyString(releases["created_at"]);
    githubRelease.published_at = copyString(releases["published_at"]);

    auto parseAuthor = [&copyString](JsonObjectConst author) -> GithubAuthor {
        GithubAuthor githubAuthor{};

        githubAuthor.login = copyString(author["login"]);
        githubAuthor.id = author["id"];
        githubAuthor.node_id = copyString(author["node_id"]);
        githubAuthor.avatar_url = copyString(author["avatar_url"]);
        githubAuthor.gravatar_id = copyString(author["gravatar_id"]);
        githubAuthor.url = copyString(author["url"]);
        githubAuthor.html_url = copyString(author["html_url"]);
        githubAuthor.followers_url = copyString(author["followers_url"]);
        githubAuthor.following_url = copyString(author["following_url"]);
        githubAuthor.gists_url = copyString(author["gists_url"]);
        githubAuthor.starred_url = copyString(author["starred_url"]);
        githubAuthor.subscriptions_url = copyString(author["subscriptions_url"]);
        githubAuthor.organizations_url = copyString(author["organizations_url"]);
        githubAuthor.repos_url = copyString(author["repos_url"]);
        githubAuthor.events_url = copyString(author["events_url"]);
        githubAuthor.received_events_url = copyString(author["received_events_url"]);
        githubAuthor.type = copyString(author["type"]);
        githubAuthor.site_admin = author["site_admin"];

        return githubAuthor;
    };
    if (releases["author"].is<JsonObject>()) {
        githubRelease.author.push_back(parseAuthor(releases["author"].as<JsonObjectConst>()));
    }

    JsonArray assets = releases["assets"].as<JsonArray>();
    bool invalidAsset = false;
    for (JsonObjectConst asset : assets) {
        const char* name = asset["name"];
        const char* url = asset["browser_download_url"];
        if (name == NULL || name[0] == '\0' || url == NULL || url[0] == '\0' || asset["id"].as<int>() <= 0) {
            invalidAsset = true;
            break;
        }
        GithubReleaseAsset githubAsset{};

        githubAsset.url = copyString(asset["url"]);
        githubAsset.id = asset["id"];
        githubAsset.node_id = copyString(asset["node_id"]);
        githubAsset.name = copyString(asset["name"]);
        githubAsset.label = copyString(asset["label"]);
        githubAsset.content_type = copyString(asset["content_type"]);
        githubAsset.state = copyString(asset["state"]);
        githubAsset.size = asset["size"];
        githubAsset.download_count = asset["download_count"];
        githubAsset.created_at = copyString(asset["created_at"]);
        githubAsset.updated_at = copyString(asset["updated_at"]);
        githubAsset.browser_download_url = copyString(asset["browser_download_url"]);
        if (asset["uploader"].is<JsonObjectConst>()) {
            githubAsset.uploader.push_back(parseAuthor(asset["uploader"].as<JsonObjectConst>()));
        }
        githubRelease.assets.push_back(std::move(githubAsset));
        if (allocationFailed) break;
    }

    if (allocationFailed || invalidAsset) {
        ESP_LOGE("GithubReleaseOTA", "Incomplete release data");
        freeRelease(githubRelease);
    }

    releases.clear();

    return githubRelease;
}

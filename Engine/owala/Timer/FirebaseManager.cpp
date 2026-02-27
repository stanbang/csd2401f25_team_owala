#include "pch.h"
#include "FirebaseManager.h"
#include "Logger.h"
#include <curl/curl.h>
#include <thread>
#include <chrono>
#include <sstream>
#include <algorithm>
#include <random>

// RapidJSON
#include <rapidjson/document.h>
#include <rapidjson/writer.h>
#include <rapidjson/stringbuffer.h>

#ifdef _WIN32
#include <windows.h>
// Undefine Windows macros that conflict with RapidJSON
#undef GetObject
#undef min
#undef max
#endif

namespace Framework {

    // --- curl write callback ---
    static size_t WriteCallback(void* contents, size_t size, size_t nmemb, std::string* output) {
        output->append((char*)contents, size * nmemb);
        return size * nmemb;
    }

    std::string FirebaseManager::GetMachineName() {
#ifdef _WIN32
        char buf[256] = {};
        DWORD len = sizeof(buf);
        GetComputerNameA(buf, &len);
        return std::string(buf);
#else
        return "unknown";
#endif
    }

    // Helper to generate a unique push ID (similar to Firebase's push IDs)
    std::string GeneratePushID() {
        static const char* PUSH_CHARS = "-0123456789ABCDEFGHIJKLMNOPQRSTUVWXYZ_abcdefghijklmnopqrstuvwxyz";
        static std::mt19937 rng(std::chrono::steady_clock::now().time_since_epoch().count());
        static std::uniform_int_distribution<int> dist(0, 63);

        std::string pushId;
        auto now = std::chrono::duration_cast<std::chrono::milliseconds>(
            std::chrono::system_clock::now().time_since_epoch()).count();

        // 8 characters for timestamp
        for (int i = 7; i >= 0; i--) {
            pushId += PUSH_CHARS[(now >> (i * 6)) & 0x3F];
        }

        // 12 random characters
        for (int i = 0; i < 12; i++) {
            pushId += PUSH_CHARS[dist(rng)];
        }

        return pushId;
    }

    void FirebaseManager::SubmitRun(const std::string& levelName, float timeSeconds) {
        std::string machineName = GetMachineName();

        // Log the submission attempt
        Logger::Get().Info(DebugType::PlayerSystem)
            << "[Firebase] Submitting run - Level: " << levelName
            << ", Time: " << timeSeconds << "s, Machine: " << machineName << std::endl;

        auto now = std::chrono::system_clock::now().time_since_epoch();
        long long timestamp = std::chrono::duration_cast<std::chrono::seconds>(now).count();

        rapidjson::StringBuffer sb;
        rapidjson::Writer<rapidjson::StringBuffer> writer(sb);
        writer.StartObject();
        writer.Key("name");      writer.String(machineName.c_str());
        writer.Key("time");      writer.Double(static_cast<double>(timeSeconds));
        writer.Key("timestamp"); writer.Int64(timestamp);
        writer.Key("level");     writer.String(levelName.c_str());
        writer.EndObject();
        std::string payload = sb.GetString();

        // Sanitize level name for URL (replace spaces with underscores)
        std::string sanitizedLevel = levelName;
        std::replace(sanitizedLevel.begin(), sanitizedLevel.end(), ' ', '_');

        // === FIXED URL STRUCTURE ===
        // Using a push ID instead of nested path to avoid 400 errors
        std::string pushId = GeneratePushID();
        std::string url = std::string(FIREBASE_URL)
            + "leaderboard/" + sanitizedLevel + "/" + pushId + ".json";

        Logger::Get().Info(DebugType::PlayerSystem) << "[Firebase] URL: " << url << std::endl;
        Logger::Get().Info(DebugType::PlayerSystem) << "[Firebase] Payload: " << payload << std::endl;

        std::thread([url, payload]() {
            CURL* curl = curl_easy_init();
            if (!curl) {
                Logger::Get().Error(DebugType::PlayerSystem) << "[Firebase] ERROR: curl_easy_init() failed" << std::endl;
                return;
            }

            // Bypass SSL verification (for development)
            curl_easy_setopt(curl, CURLOPT_SSL_VERIFYPEER, 0L);
            curl_easy_setopt(curl, CURLOPT_SSL_VERIFYHOST, 0L);

            struct curl_slist* headers = nullptr;
            headers = curl_slist_append(headers, "Content-Type: application/json");

            std::string response;
            curl_easy_setopt(curl, CURLOPT_URL, url.c_str());
            curl_easy_setopt(curl, CURLOPT_CUSTOMREQUEST, "PUT");  // PUT with unique ID works fine
            curl_easy_setopt(curl, CURLOPT_POSTFIELDS, payload.c_str());
            curl_easy_setopt(curl, CURLOPT_POSTFIELDSIZE, (long)payload.size());
            curl_easy_setopt(curl, CURLOPT_HTTPHEADER, headers);
            curl_easy_setopt(curl, CURLOPT_TIMEOUT, 10L);
            curl_easy_setopt(curl, CURLOPT_WRITEFUNCTION, WriteCallback);
            curl_easy_setopt(curl, CURLOPT_WRITEDATA, &response);

            CURLcode result = curl_easy_perform(curl);

            long httpCode = 0;
            curl_easy_getinfo(curl, CURLINFO_RESPONSE_CODE, &httpCode);

            if (result != CURLE_OK) {
                Logger::Get().Error(DebugType::PlayerSystem)
                    << "[Firebase] CURL ERROR: " << curl_easy_strerror(result) << std::endl;
            }
            else {
                if (httpCode >= 200 && httpCode < 300) {
                    Logger::Get().Info(DebugType::PlayerSystem)
                        << "[Firebase] SUCCESS! HTTP " << httpCode << " | Response: " << response << std::endl;
                }
                else {
                    Logger::Get().Error(DebugType::PlayerSystem)
                        << "[Firebase] HTTP ERROR " << httpCode << " | Response: " << response << std::endl;
                }
            }

            curl_slist_free_all(headers);
            curl_easy_cleanup(curl);
            }).detach();
    }

    void FirebaseManager::FetchLeaderboard(const std::string& levelName,
        std::function<void(std::vector<LeaderboardEntry>)> onComplete) {

        std::string sanitizedLevel = levelName;
        std::replace(sanitizedLevel.begin(), sanitizedLevel.end(), ' ', '_');

        // Simplified URL for fetching
        std::string url = std::string(FIREBASE_URL) + "leaderboard/" + sanitizedLevel + ".json";

        Logger::Get().Info(DebugType::PlayerSystem) << "[Firebase] Fetching leaderboard for: " << levelName << std::endl;

        std::thread([this, url, onComplete]() {
            CURL* curl = curl_easy_init();
            if (!curl) {
                Logger::Get().Error(DebugType::PlayerSystem) << "[Firebase] ERROR: curl_easy_init() failed for fetch" << std::endl;
                return;
            }

            // Bypass SSL verification (for development)
            curl_easy_setopt(curl, CURLOPT_SSL_VERIFYPEER, 0L);
            curl_easy_setopt(curl, CURLOPT_SSL_VERIFYHOST, 0L);

            std::string response;
            curl_easy_setopt(curl, CURLOPT_URL, url.c_str());
            curl_easy_setopt(curl, CURLOPT_WRITEFUNCTION, WriteCallback);
            curl_easy_setopt(curl, CURLOPT_WRITEDATA, &response);
            curl_easy_setopt(curl, CURLOPT_TIMEOUT, 10L);

            CURLcode res = curl_easy_perform(curl);

            long httpCode = 0;
            curl_easy_getinfo(curl, CURLINFO_RESPONSE_CODE, &httpCode);

            curl_easy_cleanup(curl);

            if (res != CURLE_OK) {
                Logger::Get().Error(DebugType::PlayerSystem)
                    << "[Firebase] Fetch CURL ERROR: " << curl_easy_strerror(res) << std::endl;
                return;
            }

            if (httpCode != 200) {
                Logger::Get().Error(DebugType::PlayerSystem)
                    << "[Firebase] Fetch HTTP Error: " << httpCode << std::endl;
                return;
            }

            std::vector<LeaderboardEntry> entries;
            rapidjson::Document doc;
            doc.Parse(response.c_str());

            if (doc.IsObject()) {
                // Structure: { "pushId": { name, time, timestamp, level } }
                for (auto& entry : doc.GetObject()) {
                    if (!entry.value.IsObject()) continue;

                    LeaderboardEntry leaderboardEntry;
                    if (entry.value.HasMember("name") && entry.value["name"].IsString())
                        leaderboardEntry.name = entry.value["name"].GetString();
                    if (entry.value.HasMember("time") && entry.value["time"].IsDouble())
                        leaderboardEntry.time = (float)entry.value["time"].GetDouble();
                    if (entry.value.HasMember("timestamp") && entry.value["timestamp"].IsInt64())
                        leaderboardEntry.timestamp = entry.value["timestamp"].GetInt64();
                    if (entry.value.HasMember("level") && entry.value["level"].IsString())
                        leaderboardEntry.level = entry.value["level"].GetString();

                    entries.push_back(leaderboardEntry);
                }
            }

            // Sort by time (ascending - fastest first)
            std::sort(entries.begin(), entries.end(),
                [](const LeaderboardEntry& a, const LeaderboardEntry& b) {
                    return a.time < b.time;
                });

            Logger::Get().Info(DebugType::PlayerSystem)
                << "[Firebase] Fetched " << entries.size() << " leaderboard entries" << std::endl;

            // Queue the callback to be called on main thread
            std::lock_guard<std::mutex> lock(m_mutex);
            m_pendingCallbacks.push_back([onComplete, entries]() {
                onComplete(entries);
                });
            }).detach();
    }

    void FirebaseManager::Update() {
        std::vector<std::function<void()>> toFire;
        {
            std::lock_guard<std::mutex> lock(m_mutex);
            toFire.swap(m_pendingCallbacks);
        }
        for (auto& cb : toFire) {
            cb();
        }
    }

}
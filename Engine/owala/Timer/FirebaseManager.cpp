#include "pch.h"
#include "FirebaseManager.h"

#include <curl/curl.h>
#include <thread>
#include <chrono>
#include <sstream>

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

    void FirebaseManager::SubmitRun(float timeSeconds) {
        std::string machineName = GetMachineName();

        // Build JSON payload
        // Stored at /leaderboard/{machineName}/{timestamp}
        auto now = std::chrono::system_clock::now().time_since_epoch();
        long long timestamp = std::chrono::duration_cast<std::chrono::seconds>(now).count();

        rapidjson::StringBuffer sb;
        rapidjson::Writer<rapidjson::StringBuffer> writer(sb);
        writer.StartObject();
        writer.Key("name");      writer.String(machineName.c_str());
        writer.Key("time");      writer.Double(static_cast<double>(timeSeconds));
        writer.Key("timestamp"); writer.Int64(timestamp);
        writer.EndObject();
        std::string payload = sb.GetString();

        // URL: /leaderboard/{machine}/{timestamp}.json
        std::string url = std::string(FIREBASE_URL)
            + "/leaderboard/" + machineName
            + "/" + std::to_string(timestamp) + ".json";

        // Fire and forget on background thread
        std::thread([url, payload]() {
            CURL* curl = curl_easy_init();
            if (!curl) return;

            struct curl_slist* headers = nullptr;
            headers = curl_slist_append(headers, "Content-Type: application/json");

            curl_easy_setopt(curl, CURLOPT_URL, url.c_str());
            curl_easy_setopt(curl, CURLOPT_CUSTOMREQUEST, "PUT");
            curl_easy_setopt(curl, CURLOPT_POSTFIELDS, payload.c_str());
            curl_easy_setopt(curl, CURLOPT_POSTFIELDSIZE, (long)payload.size());
            curl_easy_setopt(curl, CURLOPT_HTTPHEADER, headers);
            curl_easy_setopt(curl, CURLOPT_TIMEOUT, 10L);

            curl_easy_perform(curl);
            curl_slist_free_all(headers);
            curl_easy_cleanup(curl);
            }).detach();
    }

    void FirebaseManager::FetchLeaderboard(std::function<void(std::vector<LeaderboardEntry>)> onComplete) {
        std::string url = std::string(FIREBASE_URL) + "/leaderboard.json";

        std::thread([this, url, onComplete]() {
            CURL* curl = curl_easy_init();
            if (!curl) return;

            std::string response;
            curl_easy_setopt(curl, CURLOPT_URL, url.c_str());
            curl_easy_setopt(curl, CURLOPT_WRITEFUNCTION, WriteCallback);
            curl_easy_setopt(curl, CURLOPT_WRITEDATA, &response);
            curl_easy_setopt(curl, CURLOPT_TIMEOUT, 10L);

            CURLcode res = curl_easy_perform(curl);
            curl_easy_cleanup(curl);

            if (res != CURLE_OK) return;

            // Parse JSON response
            // Structure: { "MACHINENAME": { "timestamp": { name, time, timestamp } } }
            std::vector<LeaderboardEntry> entries;

            rapidjson::Document doc;
            doc.Parse(response.c_str());
            if (doc.IsObject()) {
                for (auto& machine : doc.GetObject()) {
                    if (!machine.value.IsObject()) continue;
                    for (auto& run : machine.value.GetObject()) {
                        if (!run.value.IsObject()) continue;
                        LeaderboardEntry entry;
                        if (run.value.HasMember("name") && run.value["name"].IsString())
                            entry.name = run.value["name"].GetString();
                        if (run.value.HasMember("time") && run.value["time"].IsDouble())
                            entry.time = (float)run.value["time"].GetDouble();
                        if (run.value.HasMember("timestamp") && run.value["timestamp"].IsInt64())
                            entry.timestamp = run.value["timestamp"].GetInt64();
                        entries.push_back(entry);
                    }
                }
            }

            // Sort by time ascending
            std::sort(entries.begin(), entries.end(),
                [](const LeaderboardEntry& a, const LeaderboardEntry& b) {
                    return a.time < b.time;
                });

            // Marshal callback back to main thread
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
        for (auto& cb : toFire) cb();
    }

}
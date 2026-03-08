#include "pch.h"
#include "FirebaseManager.h"

#include <thread>
#include <chrono>
#include <sstream>
#include "PlayerNameManager.h"
#include "GameStateFlags.h"
#include "Logger.h"          // Add this include
// RapidJSON
#include <rapidjson/document.h>
#include <rapidjson/writer.h>
#include <rapidjson/stringbuffer.h>

// WinHTTP - part of the Windows SDK, no extra dependencies needed
#ifdef _WIN32
#include <windows.h>
#include <winhttp.h>
#pragma comment(lib, "winhttp.lib")
#undef GetObject
#undef min
#undef max
#endif

namespace Framework {

    // -----------------------------------------------------------------------
    // WinHTTP helper: sends an HTTP request and returns the response body.
    // -----------------------------------------------------------------------
    static std::string WinHttpRequest(
        const std::string& url,
        const std::wstring& method,
        const std::string& body = "")
    {
        std::string result;

#ifdef _WIN32
        URL_COMPONENTS uc = {};
        uc.dwStructSize = sizeof(uc);

        wchar_t scheme[32] = {};
        wchar_t host[256] = {};
        wchar_t urlPath[1024] = {};

        uc.lpszScheme = scheme;   uc.dwSchemeLength = _countof(scheme);
        uc.lpszHostName = host;     uc.dwHostNameLength = _countof(host);
        uc.lpszUrlPath = urlPath;  uc.dwUrlPathLength = _countof(urlPath);

        std::wstring wurl(url.begin(), url.end());
        if (!WinHttpCrackUrl(wurl.c_str(), 0, 0, &uc)) return result;

        bool isHttps = (uc.nScheme == INTERNET_SCHEME_HTTPS);

        HINTERNET hSession = WinHttpOpen(
            L"owala/1.0",
            WINHTTP_ACCESS_TYPE_DEFAULT_PROXY,
            WINHTTP_NO_PROXY_NAME,
            WINHTTP_NO_PROXY_BYPASS, 0);
        if (!hSession) return result;

        HINTERNET hConnect = WinHttpConnect(hSession, host, uc.nPort, 0);
        if (!hConnect) { WinHttpCloseHandle(hSession); return result; }

        DWORD flags = isHttps ? WINHTTP_FLAG_SECURE : 0;
        HINTERNET hRequest = WinHttpOpenRequest(
            hConnect, method.c_str(), urlPath,
            nullptr, WINHTTP_NO_REFERER,
            WINHTTP_DEFAULT_ACCEPT_TYPES, flags);
        if (!hRequest) {
            WinHttpCloseHandle(hConnect);
            WinHttpCloseHandle(hSession);
            return result;
        }

        if (!body.empty()) {
            WinHttpAddRequestHeaders(hRequest,
                L"Content-Type: application/json",
                (ULONG)-1L,
                WINHTTP_ADDREQ_FLAG_ADD);
        }

        BOOL sent = WinHttpSendRequest(
            hRequest,
            WINHTTP_NO_ADDITIONAL_HEADERS, 0,
            body.empty() ? nullptr : (LPVOID)body.c_str(),
            (DWORD)body.size(),
            (DWORD)body.size(), 0);

        if (sent) {
            WinHttpReceiveResponse(hRequest, nullptr);

            DWORD bytesAvail = 0;
            while (WinHttpQueryDataAvailable(hRequest, &bytesAvail) && bytesAvail > 0) {
                std::string chunk(bytesAvail, '\0');
                DWORD bytesRead = 0;
                WinHttpReadData(hRequest, chunk.data(), bytesAvail, &bytesRead);
                chunk.resize(bytesRead);
                result += chunk;
            }
        }

        WinHttpCloseHandle(hRequest);
        WinHttpCloseHandle(hConnect);
        WinHttpCloseHandle(hSession);
#endif
        return result;
    }

    // -----------------------------------------------------------------------

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

    // Updated section only — replace SubmitRun() in your FirebaseManager.cpp

    // Add this include at the top of FirebaseManager.cpp:
    //   #include "PlayerNameManager.h"

    void FirebaseManager::SubmitRun(const std::string& levelName, float timeSeconds) {
        Logger::Get().Info(DebugType::FireBaseManager)
            << "[Firebase] ===== SUBMIT RUN CALLED =====" << std::endl;
        Logger::Get().Info(DebugType::FireBaseManager)
            << "[Firebase] Level: " << levelName << ", Time: " << timeSeconds << "s" << std::endl;

        // CRITICAL: Check if the player wanted to record this time
        Logger::Get().Info(DebugType::FireBaseManager)
            << "[Firebase] GameStateFlags::shouldRecordTime = "
            << (GameStateFlags::shouldRecordTime ? "true" : "false") << std::endl;

        if (!GameStateFlags::shouldRecordTime) {
            Logger::Get().Info(DebugType::FireBaseManager)
                << "[Firebase] Skipping submission - player chose not to record" << std::endl;
            return;
        }

        // Use player's chosen name; fall back to machine name if none set
        std::string playerName = Framework::PlayerNameManager::Get().HasName()
            ? Framework::PlayerNameManager::Get().GetName()
            : GetMachineName();

        Logger::Get().Info(DebugType::FireBaseManager)
            << "[Firebase] Player name: '" << playerName << "'" << std::endl;

        // Don't submit if name is empty
        if (playerName.empty()) {
            Logger::Get().Info(DebugType::FireBaseManager)
                << "[Firebase] Skipping submission - player name is empty" << std::endl;
            return;
        }

        Logger::Get().Info(DebugType::FireBaseManager)
            << "[Firebase] Submitting run for level: " << levelName
            << ", player: " << playerName << ", time: " << timeSeconds << "s" << std::endl;

        auto now = std::chrono::system_clock::now().time_since_epoch();
        long long timestamp = std::chrono::duration_cast<std::chrono::seconds>(now).count();

        // Build JSON payload
        rapidjson::StringBuffer sb;
        rapidjson::Writer<rapidjson::StringBuffer> writer(sb);
        writer.StartObject();
        writer.Key("name");      writer.String(playerName.c_str());
        writer.Key("level");     writer.String(levelName.c_str());
        writer.Key("time");      writer.Double(static_cast<double>(timeSeconds));
        writer.Key("timestamp"); writer.Int64(timestamp);
        writer.EndObject();
        std::string payload = sb.GetString();

        // URL: /leaderboard/{levelName}/{playerName}/{timestamp}.json
        std::string url = std::string(FIREBASE_URL)
            + "/leaderboard/" + levelName
            + "/" + playerName
            + "/" + std::to_string(timestamp) + ".json";

        Logger::Get().Info(DebugType::FireBaseManager)
            << "[Firebase] Sending to URL: " << url << std::endl;

        std::thread([url, payload]() {
            std::string response = WinHttpRequest(url, L"PUT", payload);
            // We can't use Logger here because it might not be thread-safe
            // But we could print to console directly for debugging
            // OutputDebugStringA(("[Firebase] Response: " + response + "\n").c_str());
            }).detach();
    }
    void FirebaseManager::FetchLeaderboard(const std::string& levelName,
        std::function<void(std::vector<LeaderboardEntry>)> onComplete)
    {
        // URL: /leaderboard/{levelName}.json — fetch only this level's entries
        std::string url = std::string(FIREBASE_URL)
            + "/leaderboard/" + levelName + ".json";

        std::thread([this, url, levelName, onComplete]() {
            std::string response = WinHttpRequest(url, L"GET");
            if (response.empty()) return;

            // Structure: { "MACHINENAME": { "timestamp": { name, level, time, timestamp } } }
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
                        if (run.value.HasMember("level") && run.value["level"].IsString())
                            entry.level = run.value["level"].GetString();
                        if (run.value.HasMember("time") && run.value["time"].IsDouble())
                            entry.time = (float)run.value["time"].GetDouble();
                        if (run.value.HasMember("timestamp") && run.value["timestamp"].IsInt64())
                            entry.timestamp = run.value["timestamp"].GetInt64();
                        entries.push_back(entry);
                    }
                }
            }

            // Sort by time ascending (fastest first)
            std::sort(entries.begin(), entries.end(),
                [](const LeaderboardEntry& a, const LeaderboardEntry& b) {
                    return a.time < b.time;
                });

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
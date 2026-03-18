/*********************************************************************//**
 * \file   FirebaseManager.cpp
 *
 * \primary_author
 *      Jordain Ng Ying Zhe (100%)
 *
 * \brief
 *      Implementation of the FirebaseManager class for cloud-based
 *      leaderboard functionality using Firebase Realtime Database.
 *
 * \details
 *      This file contains the implementation of all FirebaseManager methods,
 *      along with internal helper functions for HTTP communication. Key
 *      features implemented include:
 *        - Asynchronous HTTP requests using WinHTTP
 *        - JSON serialization/deserialization with RapidJSON
 *        - Thread-safe callback marshaling
 *        - Error handling and logging
 *
 *      The WinHttpRequest helper function handles all low-level HTTP
 *      communication, supporting both GET and PUT methods with proper
 *      SSL/TLS handling for HTTPS endpoints.
 *
 * \copyright
 *      Copyright (C) 2025 DigiPen Institute of Technology.
 *      Reproduction or disclosure of this file or its contents without
 *      the prior written consent of DigiPen Institute of Technology is prohibited.
 *********************************************************************/

#include "pch.h"
#include "FirebaseManager.h"

#include <thread>
#include <chrono>
#include <sstream>
#include "Logger.h"

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
    /**
     * @brief Internal helper function to send HTTP requests using WinHTTP.
     *
     * This function handles the complete lifecycle of an HTTP request including
     * URL parsing, connection establishment, request sending, and response
     * reading. It supports both HTTP and HTTPS endpoints with proper SSL
     * handling for secure connections.
     *
     * @param url    The complete URL to send the request to
     * @param method HTTP method as wide string (L"GET", L"PUT", etc.)
     * @param body   Optional request body for PUT/POST requests (JSON string)
     * @return std::string The response body from the server, or empty on failure
     *
     * @note This function is Windows-specific and uses the WinHTTP API.
     * @warning Returns empty string on any error - caller must check for empty response.
     *
     * Implementation details:
     * 1. Parses URL into components (scheme, host, port, path)
     * 2. Creates WinHTTP session and connection handles
     * 3. Opens request with appropriate SSL flags for HTTPS
     * 4. Adds Content-Type header for requests with body
     * 5. Sends request and reads response in chunks
     * 6. Cleans up all handles before returning
     */
    static std::string WinHttpRequest(
        const std::string& url,         // Full target URL
		const std::wstring& method,     // HTTP method (L"GET", L"PUT", etc.)
		const std::string& body = "")   // Optional body for PUT/POST requests
    {
		std::string result; // Response body to return, empty on failure
#ifdef _WIN32
		URL_COMPONENTS uc = {}; // Zero-initialize before setting dwStructSize
		uc.dwStructSize = sizeof(uc); // Must set this for WinHttpCrackUrl to work, this is how it knows which version of the structure we're using

		wchar_t scheme[32] = {};    // Buffers for URL components - must be large enough to hold expected values
		wchar_t host[256] = {};     // Note: WinHTTP limits host name to 255 chars, so 256 is safe for null-terminator
		wchar_t urlPath[1024] = {}; // URL path can be long, but 1024 should be sufficient for our use case

		uc.lpszScheme = scheme;  uc.dwSchemeLength = _countof(scheme);  // Set pointers and lengths for the components we want to extract
		uc.lpszHostName = host;    uc.dwHostNameLength = _countof(host);    // Port is extracted as a number, no buffer needed
		uc.lpszUrlPath = urlPath; uc.dwUrlPathLength = _countof(urlPath);   // We only care about scheme, host, port, and path for our requests. User info and extra info are not needed.

		std::wstring wurl(url.begin(), url.end());  // Convert URL to wide string for WinHTTP functions
		if (!WinHttpCrackUrl(wurl.c_str(), 0, 0, &uc)) return result;   // Parse the URL into components using WinHttpCrackUrl. Returns false on failure.

		bool isHttps = (uc.nScheme == INTERNET_SCHEME_HTTPS);   // Determine if the URL is HTTPS for setting secure flags later

		// Create WinHTTP session, this is the first step in using WinHTTP. The user agent string can be anything, we just use "owala/1.0" for identification. We use the default proxy settings from the system. Returns NULL on failure.
		HINTERNET hSession = WinHttpOpen(L"owala/1.0",          // User agent string
			WINHTTP_ACCESS_TYPE_DEFAULT_PROXY,                  // Use default proxy settings from the system
			WINHTTP_NO_PROXY_NAME, WINHTTP_NO_PROXY_BYPASS, 0); // Open a WinHTTP session. Returns NULL on failure.
		if (!hSession) return result;   // If session creation failed, return empty result

		HINTERNET hConnect = WinHttpConnect(hSession, host, uc.nPort, 0);   // Establish a connection to the server using the host and port from the URL. Returns NULL on failure.
        if (!hConnect) {
			WinHttpCloseHandle(hSession);   // If connection failed, clean up session handle before returning
            return result;
        }

		DWORD     flags = isHttps ? WINHTTP_FLAG_SECURE : 0;    // Set secure flag for HTTPS requests
		HINTERNET hRequest = WinHttpOpenRequest(    // Open an HTTP request handle for the specified method and URL path. The full URL is not used here, only the path, because the connection handle already knows the host. Returns NULL on failure.
            hConnect, method.c_str(), urlPath,
            nullptr, WINHTTP_NO_REFERER,
            WINHTTP_DEFAULT_ACCEPT_TYPES, flags);
        if (!hRequest) {
			WinHttpCloseHandle(hConnect);   // If request handle creation failed, clean up connection and session handles before returning
            WinHttpCloseHandle(hSession);
            return result;
        }

		// If we have a body to send (for PUT/POST), add the Content-Type header for JSON. We use WINHTTP_ADDREQ_FLAG_ADD to add this header without replacing any existing headers (though there shouldn't be any at this point). The length of the header string is -1L to indicate it's null-terminated.
        if (!body.empty()) {
            WinHttpAddRequestHeaders(hRequest,
                L"Content-Type: application/json",
                (ULONG)-1L, WINHTTP_ADDREQ_FLAG_ADD);
        }

		// Send the request with the body if provided. The total length is the same as the optional length since we're sending it all at once. Returns false on failure.
        BOOL sent = WinHttpSendRequest(hRequest,
            WINHTTP_NO_ADDITIONAL_HEADERS, 0,
            body.empty() ? nullptr : (LPVOID)body.c_str(),
            (DWORD)body.size(), (DWORD)body.size(), 0);

		if (sent) { // If the request was sent successfully, we can now receive the response. This will block until the response is received, but since this function is called on a background thread, it won't affect the main game loop.
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
    // Public Interface Implementation
    // -----------------------------------------------------------------------

    /**
     * @brief Submits a new run to the Firebase leaderboard asynchronously.
     *
     * This method constructs a JSON payload with the run data and sends it
     * to Firebase via a PUT request. The operation runs on a detached thread
     * to avoid blocking the main game loop.
     *
     * Database path: /leaderboard/{levelName}/{playerName}/{timestamp}.json
     *
     * @param levelName   Name of the level where the run occurred
     * @param playerName  Name of the player who achieved the run
     * @param timeSeconds Completion time in seconds
     *
     * @note The method performs validation checks:
     *       - Verifies database URL is set (logs error if not)
     *       - Skips submission if player name is empty (logs warning)
     * @warning No callback is provided for submission completion - failures
     *          are only logged, not reported back to the game.
     */
    void FirebaseManager::SubmitRun(const std::string& levelName,
        const std::string& playerName,
        float              timeSeconds)
    {
        Logger::Get().Info(DebugType::FireBaseManager)
            << "[Firebase] SubmitRun - level: " << levelName
            << "  player: " << playerName
            << "  time: " << timeSeconds << "s" << std::endl;

        if (m_databaseUrl.empty()) {
            Logger::Get().Info(DebugType::FireBaseManager)
                << "[Firebase] ERROR - database URL not set. Call SetDatabaseUrl() at startup." << std::endl;
            return;
        }

        if (playerName.empty()) {
            Logger::Get().Info(DebugType::FireBaseManager)
                << "[Firebase] Skipping - player name is empty." << std::endl;
            return;
        }

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
        std::string url = m_databaseUrl
            + "/leaderboard/" + levelName
            + "/" + playerName
            + "/" + std::to_string(timestamp) + ".json";

        Logger::Get().Info(DebugType::FireBaseManager)
            << "[Firebase] PUT -> " << url << std::endl;

        std::thread([url, payload]() {
            WinHttpRequest(url, L"PUT", payload);
            }).detach();
    }

    /**
     * @brief Fetches the leaderboard for a specific level asynchronously.
     *
     * This method sends a GET request to Firebase for all entries under the
     * specified level. The response is parsed from JSON, sorted by time
     * (ascending), and passed to the provided callback on the main thread
     * via the Update() method.
     *
     * Database path: /leaderboard/{levelName}.json
     *
     * @param levelName   Name of the level to fetch leaderboard for
     * @param onComplete  Callback function that receives the sorted entries
     *
     * @note The callback is guaranteed to execute on the main thread when
     *       Update() is called. If the network request fails or returns
     *       empty data, the callback is not invoked.
     *
     * JSON structure expected from Firebase:
     * {
     *   "player1": {
     *     "timestamp1": { "name": "player1", "level": "Level1", "time": 45.67, "timestamp": 1234567890 },
     *     "timestamp2": { ... }
     *   },
     *   "player2": { ... }
     * }
     */
    void FirebaseManager::FetchLeaderboard(const std::string& levelName,
        std::function<void(std::vector<LeaderboardEntry>)> onComplete)
    {
        std::string url = m_databaseUrl + "/leaderboard/" + levelName + ".json";

        std::thread([this, url, onComplete]() {
            std::string response = WinHttpRequest(url, L"GET");
            if (response.empty()) return;

            std::vector<LeaderboardEntry> entries;
            rapidjson::Document doc;
            doc.Parse(response.c_str());

            if (doc.IsObject()) {
                // Iterate through players (first level keys)
                for (auto& player : doc.GetObject()) {
                    if (!player.value.IsObject()) continue;

                    // Iterate through runs for this player (timestamp keys)
                    for (auto& run : player.value.GetObject()) {
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

            // Sort entries by time (ascending - fastest first)
            std::sort(entries.begin(), entries.end(),
                [](const LeaderboardEntry& a, const LeaderboardEntry& b) {
                    return a.time < b.time;
                });

            // Queue callback for main thread execution
            std::lock_guard<std::mutex> lock(m_mutex);
            m_pendingCallbacks.push_back([onComplete, entries]() {
                onComplete(entries);
                });
            }).detach();
    }

    /**
     * @brief Processes all pending callbacks on the main thread.
     *
     * This method must be called every frame from the game's main loop.
     * It safely transfers callbacks from the background thread queue to
     * the main thread and executes them in order.
     *
     * The method uses a double-buffering technique via swap() to minimize
     * lock duration - the mutex is only held while swapping the queues,
     * not during callback execution.
     *
     * @note This method is NOT thread-safe and should ONLY be called from
     *       the main thread.
     */
    void FirebaseManager::Update() {
        std::vector<std::function<void()>> toFire;
        {
            // Minimize lock duration - just swap queues
            std::lock_guard<std::mutex> lock(m_mutex);
            toFire.swap(m_pendingCallbacks);
        }

        // Execute all callbacks on main thread
        for (auto& cb : toFire) {
            cb();
        }
    }

} // namespace Framework
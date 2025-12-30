#ifndef SERVERHOP_HPP
#define SERVERHOP_HPP

#include <string>
#include <vector>
#include <algorithm>
#include <curl/curl.h>

#include "json.hpp"

using json = nlohmann::json;

namespace ServerHopper {
    // state
    static std::vector<std::string> bannedServerIDs;
    static std::string currentCursor = "";
    static int minFreeSlots = 2;
    static bool isHopping = false;
    static std::string lastError = "";
    static std::string lastFoundServer = "";

    // callback for curl
    static size_t WriteCallback(void* contents, size_t size, size_t nmemb, void* userp) {
        size_t total_size = size * nmemb;
        std::string* buffer = static_cast<std::string*>(userp);
        buffer->append(static_cast<char*>(contents), total_size);
        return total_size;
    }

    // check if server is banned
    inline bool IsServerBanned(const std::string& serverID) {
        return std::find(bannedServerIDs.begin(), bannedServerIDs.end(), serverID) != bannedServerIDs.end();
    }

    // add server to banned list
    inline void BanServer(const std::string& serverID) {
        if (!IsServerBanned(serverID)) {
            bannedServerIDs.push_back(serverID);
        }
    }

    // remove server from banned list
    inline void UnbanServer(const std::string& serverID) {
        auto it = std::find(bannedServerIDs.begin(), bannedServerIDs.end(), serverID);
        if (it != bannedServerIDs.end()) {
            bannedServerIDs.erase(it);
        }
    }

    // clear all banned servers
    inline void ClearBannedServers() {
        bannedServerIDs.clear();
        currentCursor = "";
    }

    // Format URL for API request
    inline std::string FormatURL(const std::string& placeID, const std::string& cursor = "") {
        std::string url = "https://games.roblox.com/v1/games/" + placeID +
                         "/servers/Public?limit=100&sortOrder=Desc&excludeFullGames=true";
        if (!cursor.empty()) {
            url += "&cursor=" + cursor;
        }
        return url;
    }

    // perform server hop
    inline bool HopToNewServer(const std::string& placeID, std::string& outServerID) {
        CURL* curl = curl_easy_init();
        if (!curl) {
            lastError = "Failed to initialize CURL";
            return false;
        }

        std::string response;
        std::string url = FormatURL(placeID, currentCursor);

        curl_easy_setopt(curl, CURLOPT_URL, url.c_str());
        curl_easy_setopt(curl, CURLOPT_WRITEFUNCTION, WriteCallback);
        curl_easy_setopt(curl, CURLOPT_WRITEDATA, &response);
        curl_easy_setopt(curl, CURLOPT_USERAGENT, "Mozilla/5.0");
        curl_easy_setopt(curl, CURLOPT_SSL_VERIFYPEER, 0L);
        curl_easy_setopt(curl, CURLOPT_SSL_VERIFYHOST, 0L);
        curl_easy_setopt(curl, CURLOPT_TIMEOUT, 10L);

        CURLcode res = curl_easy_perform(curl);
        curl_easy_cleanup(curl);

        if (res != CURLE_OK) {
            lastError = std::string("CURL error: ") + curl_easy_strerror(res);
            return false;
        }

        try {
            json j = json::parse(response);

            // update cursor
            if (j.contains("nextPageCursor") && !j["nextPageCursor"].is_null()) {
                currentCursor = j["nextPageCursor"].get<std::string>();
            } else {
                currentCursor = "";
            }

            // Find suitable server
            if (j.contains("data") && j["data"].is_array()) {
                for (const auto& server : j["data"]) {
                    if (!server.contains("id")) continue;

                    std::string serverID = server["id"].get<std::string>();

                    // Skip banned servers
                    if (IsServerBanned(serverID)) continue;

                    // check free slots
                    int playing = server.contains("playing") ? server["playing"].get<int>() : 0;
                    int maxPlayers = server.contains("maxPlayers") ? server["maxPlayers"].get<int>() : 0;
                    int freeSlots = maxPlayers - playing;

                    if (freeSlots >= minFreeSlots) {
                        // Found a good server!
                        outServerID = serverID;
                        BanServer(serverID);
                        lastFoundServer = serverID;
                        lastError = "";
                        return true;
                    }
                }

                lastError = "No suitable servers found on this page";
                return false;
            }

            lastError = "No server data in response";
            return false;

        } catch (const json::exception& e) {
            lastError = std::string("JSON error: ") + e.what();
            return false;
        }
    }
}

#endif // SERVERHOP_HPP

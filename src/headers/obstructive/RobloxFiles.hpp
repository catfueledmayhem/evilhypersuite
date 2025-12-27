#pragma once
#include <string>
#ifdef __linux__
#include <pwd.h>
#include <unistd.h>
#include <sys/types.h>
#endif
#include <cstdlib>
#include <regex>
#include <iostream>
#include <fstream>
#include <filesystem>

#include "logzz.hpp"
namespace fs = std::filesystem;

inline std::string getRobloxAppDataDirectory() {
#ifdef _WIN32
    if (const char* localAppData = std::getenv("LOCALAPPDATA")) {
        return std::string(localAppData) + "\\Roblox";
    } else {
        return "C:\\Users\\Default\\AppData\\Local\\Roblox";
    }
#else
    const char* home = nullptr;

    // If running under sudo, get original user's home directory
    if (const char* sudoUser = std::getenv("SUDO_USER")) {
        if (struct passwd* pw = getpwnam(sudoUser)) {
            home = pw->pw_dir;
        }
    }

    // Fallback to normal HOME
    if (!home) home = std::getenv("HOME");

    // Final fallback
    if (!home) home = "/home/username";

    // Flatpak Sober default
    return std::string(home) + "/.var/app/org.vinegarhq.Sober/data/sober/appData";
#endif
}

inline std::string RobloxAppDataDirectory = getRobloxAppDataDirectory();

inline std::string getFirstLogFile(const std::string& dir) {
    return logzz::last_log_file;
}

inline unsigned long long getLastPlaceID() {
    return logzz::current_place_ID;
}

inline std::string getInstanceIDFromLog(const std::string& logFilePath) {
    std::cout << "[DEBUG] Opening log file for InstanceID: " << logFilePath << std::endl;

    std::ifstream file(logFilePath);
    if (!file.is_open()) {
        std::cout << "[DEBUG] Failed to open file!" << std::endl;
        return "";
    }

    std::string line;
    // Pattern: Joining game 'e2f4d0cb-fe07-4eb1-b905-71b61dffd170'
    std::regex instanceid_regex(R"(Joining game '([a-fA-F0-9\-]+)')", std::regex_constants::icase);
    std::string lastInstanceId = "";
    int lineNum = 0;
    int matchCount = 0;

    while (std::getline(file, line)) {
        lineNum++;
        std::smatch match;
        if (std::regex_search(line, match, instanceid_regex)) {
            matchCount++;
            if (match.size() > 1) {
                lastInstanceId = match[1].str();
                std::cout << "[DEBUG] Line " << lineNum << " - Found InstanceID: " << lastInstanceId << std::endl;
                std::cout << "[DEBUG] Full line: " << line << std::endl;
            }
        }
    }

    std::cout << "[DEBUG] Total lines read: " << lineNum << std::endl;
    std::cout << "[DEBUG] Total matches: " << matchCount << std::endl;
    std::cout << "[DEBUG] Final InstanceID: " << lastInstanceId << std::endl;

    return lastInstanceId;
}

inline std::string getLastInstanceID() {
    std::cout << "[DEBUG] Roblox AppData Directory: " << RobloxAppDataDirectory << std::endl;

    // Look in the 'logs' subdirectory
    std::string logsDir = RobloxAppDataDirectory +
#ifdef _WIN32
        "\\logs";
#else
        "/logs";
#endif

    std::cout << "[DEBUG] Logs Directory: " << logsDir << std::endl;

    std::string last_log = getFirstLogFile(logsDir);
    std::cout << "[DEBUG] Log file found: " << (last_log.empty() ? "NONE" : last_log) << std::endl;

    if (last_log.empty()) return "";
    return getInstanceIDFromLog(last_log);
}

#pragma once

#ifdef _WIN32
#ifndef HELPER_WINDOWS_INCLUDED
#define HELPER_WINDOWS_INCLUDED
#define WIN32_LEAN_AND_MEAN
#ifndef NOMINMAX
#define NOMINMAX
#endif
#ifndef _WIN32_WINNT
#define _WIN32_WINNT 0x0600
#endif
#define Rectangle Win32Rectangle
#define CloseWindow Win32CloseWindow
#define ShowCursor Win32ShowCursor
#define DrawText Win32DrawText
#define DrawTextEx Win32DrawTextEx
#define LoadImage Win32LoadImage
#include <windows.h>
#include <shellapi.h>
#include <sddl.h>
#include <tlhelp32.h>
#undef Rectangle
#undef CloseWindow
#undef ShowCursor
#undef DrawText
#undef DrawTextEx
#undef LoadImage
#endif
#else
#include <unistd.h>
#include <pwd.h>
#include <sys/types.h>
#endif

#include <iostream>
#include <string>
#include <cstdlib>
#include <stdlib.h>
#include <fstream>
#include <filesystem>
#include <chrono>
#include <iomanip>
#include <sstream>
#include <algorithm>
#include "Globals.hpp"
#include "inpctrl.hpp"

namespace fs = std::filesystem;

inline bool isElevated() {
#if defined(_WIN32)
    HANDLE token = NULL;
    if (OpenProcessToken(GetCurrentProcess(), TOKEN_QUERY, &token)) {
        TOKEN_ELEVATION elevation;
        DWORD returned = 0;
        BOOL ok = GetTokenInformation(token, TokenElevation, &elevation, sizeof(elevation), &returned);
        CloseHandle(token);
        if (ok && returned == sizeof(elevation)) {
            return elevation.TokenIsElevated != 0;
        }
    }

    BOOL isAdmin = FALSE;
    PSID adminGroup = NULL;
    SID_IDENTIFIER_AUTHORITY ntAuth = SECURITY_NT_AUTHORITY;
    if (AllocateAndInitializeSid(&ntAuth, 2,
                                 SECURITY_BUILTIN_DOMAIN_RID,
                                 DOMAIN_ALIAS_RID_ADMINS,
                                 0,0,0,0,0,0,
                                 &adminGroup)) {
        CheckTokenMembership(NULL, adminGroup, &isAdmin);
        FreeSid(adminGroup);
    }
    return isAdmin != FALSE;
#else
    return geteuid() == 0;
#endif
}

#if defined(__linux__)

inline std::string findPrivEscTool() {
    // Ordered by preference
    static const char* candidates[] = { "sudo", "doas", "pkexec", "su", nullptr };
    for (int i = 0; candidates[i]; i++) {
        std::string check = std::string("command -v ") + candidates[i] + " >/dev/null 2>&1";
        if (system(check.c_str()) == 0) {
            return candidates[i];
        }
    }
    return "sudo"; // fallback
}

inline std::string privEscPrefix(const std::string& tool,
                                  const std::string& user,
                                  bool withPassword = false,
                                  const std::string& password = "") {
    if (tool == "sudo") {
        std::string prefix;
        if (withPassword && !password.empty()) {
            prefix = "echo \"" + password + "\" | sudo -S -p '' ";
        } else {
            prefix = "sudo ";
        }
        if (!user.empty()) prefix += "-u " + user + " ";
        return prefix;
    } else if (tool == "doas") {
        std::string prefix = "doas ";
        if (!user.empty()) prefix += "-u " + user + " ";
        return prefix;
    } else if (tool == "pkexec") {
        std::string prefix = "pkexec ";
        if (!user.empty()) prefix += "--user " + user + " ";
        return prefix;
    } else if (tool == "su") {
        // su <user> -c "command"  — wraps differently, caller must quote the command
        if (!user.empty()) return "su " + user + " -c ";
        return "su -c ";
    }
    return "sudo ";
}

inline std::string getNormalUser() {
    // sudo sets SUDO_USER
    const char* u = getenv("SUDO_USER");
    if (u && u[0]) return u;

    // doas sets DOAS_USER
    u = getenv("DOAS_USER");
    if (u && u[0]) return u;

    // pkexec sets PKEXEC_UID — resolve to username
    const char* pkuid = getenv("PKEXEC_UID");
    if (pkuid && pkuid[0]) {
        uid_t uid = (uid_t)atoi(pkuid);
        struct passwd* pw = getpwuid(uid);
        if (pw) return pw->pw_name;
    }

    // Non-elevated or unknown escalation tool — just use current user
    u = getenv("USER");
    if (u && u[0]) return u;

    // Last resort
    struct passwd* pw = getpwuid(getuid());
    if (pw) return pw->pw_name;

    return "";
}

inline bool hasX11Display() {
    const char* d = getenv("DISPLAY");
    return d && d[0] != '\0';
}

inline void runXhostPlus() {
    if (!hasX11Display()) return;

    if (isElevated()) {
        std::string tool = findPrivEscTool();
        std::string user = getNormalUser();
        std::string prefix = privEscPrefix(tool, user);
        std::string cmd = prefix + "xhost +";
        system(cmd.c_str());
    } else {
        system("xhost +");
    }
}
#endif

inline int file_exists(const char *path) {
    FILE *f = fopen(path, "rb");
    if (!f) return 0;
    fclose(f);
    return 1;
}

inline bool TryElevate(const char* password)
{
    if (isElevated()) return true;

#if defined(__linux__)
    char exePath[4096] = {0};
    readlink("/proc/self/exe", exePath, sizeof(exePath)-1);

    std::string tool = findPrivEscTool();
    std::string testCmd;
    if (tool == "sudo") {
        testCmd = "echo \"" + std::string(password) + "\" | sudo -S -p '' true 2>&1";
    } else if (tool == "doas") {
        testCmd = "doas true 2>&1";
    } else if (tool == "pkexec") {
        testCmd = "pkexec true 2>&1";
    } else {
        testCmd = "true"; // su is interactive, skip pre-test
    }

    if (system(testCmd.c_str()) != 0) return false;

    // Re-launch elevated
    std::string cmd;
    if (tool == "sudo") {
        cmd = "echo \"" + std::string(password) + "\" | sudo -S -p '' \"" +
              std::string(exePath) + "\" &";
    } else if (tool == "doas") {
        cmd = "doas \"" + std::string(exePath) + "\" &";
    } else if (tool == "pkexec") {
        cmd = "pkexec \"" + std::string(exePath) + "\" &";
    } else {
        // su: prompt will appear in terminal
        cmd = "su -c '\"" + std::string(exePath) + "\"' &";
    }

    system(cmd.c_str());
    usleep(100000); // 100ms for the new process to start
    exit(0);
#else
    return false;
#endif
}

inline bool isProcessRunning(const std::string& nameSubstring) {
#ifdef _WIN32
    bool found = false;
    HANDLE hSnapshot = CreateToolhelp32Snapshot(TH32CS_SNAPPROCESS, 0);
    if (hSnapshot == INVALID_HANDLE_VALUE) return false;

    PROCESSENTRY32 pe;
    pe.dwSize = sizeof(PROCESSENTRY32);

    if (Process32First(hSnapshot, &pe)) {
        do {
            std::string exeName = pe.szExeFile;
            std::transform(exeName.begin(), exeName.end(), exeName.begin(), ::tolower);

            std::string lowerName = nameSubstring;
            std::transform(lowerName.begin(), lowerName.end(), lowerName.begin(), ::tolower);

            if (exeName.find(lowerName) != std::string::npos) {
                found = true;
                break;
            }
        } while (Process32Next(hSnapshot, &pe));
    }

    CloseHandle(hSnapshot);
    return found;
#else
    FILE* pipe = popen("pgrep -a ''", "r");
    if (!pipe) return false;

    char buffer[256];
    bool found = false;
    std::string lowerName = nameSubstring;
    std::transform(lowerName.begin(), lowerName.end(), lowerName.begin(), ::tolower);

    while (fgets(buffer, sizeof(buffer), pipe)) {
        std::string line(buffer);
        std::transform(line.begin(), line.end(), line.begin(), ::tolower);
        if (line.find(lowerName) != std::string::npos) {
            found = true;
            break;
        }
    }

    pclose(pipe);
    return found;
#endif
}

inline void restartRoblox() {
#ifdef _WIN32
    system("taskkill /IM RobloxPlayerBeta.exe /F");

    std::string url;
    bool hasPlaceId = strlen(placeIdBuffer) > 0;
    bool hasInstanceId = strlen(instanceIdBuffer) > 0;

    if (hasPlaceId) {
        url = "roblox://experiences/start?placeId=" + std::string(placeIdBuffer);
        if (hasInstanceId) {
            url += "&gameInstanceId=" + std::string(instanceIdBuffer);
        }
    } else {
        url = "roblox://";
    }

    ShellExecuteA(nullptr, "open", url.c_str(), nullptr, nullptr, SW_SHOWNORMAL);
#else
    std::string normalUser = getNormalUser();
    if (normalUser.empty()) return;

    std::string tool = findPrivEscTool();
    std::string prefix;
    if (isElevated()) {
        prefix = privEscPrefix(tool, normalUser);
    }

    std::string killCmd = prefix + "bash -c 'killall -9 " + roblox_process_name + " >/dev/null 2>&1'";
    std::system(killCmd.c_str());

    std::string url;
    bool hasPlaceId   = strlen(placeIdBuffer)    > 0;
    bool hasInstanceId = strlen(instanceIdBuffer) > 0;

    if (hasPlaceId) {
        url = "roblox://experiences/start?placeId=" + std::string(placeIdBuffer);
        if (hasInstanceId) {
            url += "&gameInstanceId=" + std::string(instanceIdBuffer);
        }
        std::string launchCmd = prefix + "bash -c 'xdg-open \"" + url + "\" &'";
        std::system(launchCmd.c_str());
    } else {
        std::string runCmd = prefix + "bash -c 'flatpak run org.vinegarhq.Sober &'";
        std::system(runCmd.c_str());
    }
#endif
}

inline std::ofstream& log_file()
{
    static std::ofstream file = [] {
        fs::create_directories("logs");

        auto now = std::chrono::system_clock::now();
        std::time_t t = std::chrono::system_clock::to_time_t(now);

        std::tm tm{};
#ifdef _WIN32
        localtime_s(&tm, &t);
#else
        localtime_r(&t, &tm);
#endif

        std::ostringstream filename;
        filename << "logs/"
                 << std::put_time(&tm, "%Y-%m-%d_%H-%M-%S")
                 << ".log";

        return std::ofstream(filename.str(), std::ios::app);
    }();

    return file;
}

inline std::string current_time_string()
{
    using namespace std::chrono;
    auto now = system_clock::now();
    auto ms = duration_cast<milliseconds>(now.time_since_epoch()) % 1000;

    std::time_t t = system_clock::to_time_t(now);
    std::tm tm{};
#ifdef _WIN32
    localtime_s(&tm, &t);
#else
    localtime_r(&t, &tm);
#endif

    std::ostringstream oss;
    oss << std::setfill('0')
        << std::setw(2) << tm.tm_hour << ':'
        << std::setw(2) << tm.tm_min << ':'
        << std::setw(2) << tm.tm_sec << '.'
        << std::setw(3) << ms.count();

    return oss.str();
}

inline void log(const std::string& text)
{
    std::string time = current_time_string();
    std::string line = "[" + time + "] [HSLOG] " + text;

    auto& file = log_file();
    file << line << '\n';
    file.flush();

    std::ofstream latest("logs/LATEST.log", std::ios::app);
    latest << line << '\n';
    latest.flush();

    std::cout << line << std::endl;
}

inline void RunSilent(const std::string &cmd) {
#ifdef _WIN32
    std::string finalCmd = cmd + " >nul 2>&1";
#else
    std::string finalCmd = cmd + " >/dev/null 2>&1";
#endif
    system(finalCmd.c_str());
}

inline void typeSlashAzerty() {
    input.holdKey(CrossInput::Key::LShift);
    std::this_thread::sleep_for(std::chrono::milliseconds(25));

    input.holdKey(CrossInput::Key::Dot);
    std::this_thread::sleep_for(std::chrono::milliseconds(65));

    input.releaseKey(CrossInput::Key::Dot);
    std::this_thread::sleep_for(std::chrono::milliseconds(20));

    input.releaseKey(CrossInput::Key::LShift);
    std::this_thread::sleep_for(std::chrono::milliseconds(20));
}

inline void showMessageBox(const std::string& title, const std::string& msg) {
#if defined(_WIN32)
    MessageBoxA(NULL, msg.c_str(), title.c_str(), MB_OK | MB_ICONINFORMATION);
#elif defined(__linux__)
    std::string cmd = "zenity --info --title=\"" + title + "\" --text=\"" + msg + "\"";
    system(cmd.c_str());
#else
    #error "Unsupported platform"
#endif
}

inline void bindToMacro(std::string macro_name) {
    if (!events[3]) {
        events[3] = true;
        CrossInput::Key userKey = input.getCurrentPressedKey(5000);
        if (userKey != static_cast<CrossInput::Key>(0)) {
            std::cout << "[3RU] [inpctrl] Bound: " << input.getKeyName(userKey) << std::endl;
            Binds[macro_name] = userKey;
        }
        events[3] = false;
    }
}

inline void BindSpamKey() {
    if (!events[8]) {
        events[8] = true;
        CrossInput::Key userKey = input.getCurrentPressedKey(5000);
        if (userKey != static_cast<CrossInput::Key>(0)) {
            std::cout << "[3RU] [inpctrl] Bound: " << input.getKeyName(userKey) << std::endl;
            SpamKey = userKey;
        }
        events[8] = false;
    }
}

inline void BindVariable(CrossInput::Key* keyLoc) {
    if (!events[9]) {
        events[9] = true;
        CrossInput::Key userKey = input.getCurrentPressedKey(5000);
        if (userKey != static_cast<CrossInput::Key>(0)) {
            std::cout << "[3RU] [inpctrl] Bound: " << input.getKeyName(userKey) << std::endl;
            *keyLoc = userKey;
        }
        events[9] = false;
    }
}

inline unsigned short GetIDFromCodeName(std::string CodeName) {
    if (CodeName == "Freeze") {
        return 0;
    } else if (CodeName == "Laugh") {
        return 1;
    } else if (CodeName == "E-Dance") {
        return 2;
    } else if (CodeName == "Lag-switch") {
        return 4;
    } else if (CodeName == "Buckey-clip") {
        return 5;
    } else if (CodeName == "Speedglitch") {
        return 6;
    } else if (CodeName == "Spam-Key") {
        return 7;
    } else if (CodeName == "Disable-Head-Collision") {
        return 10;
    } else if (CodeName == "NHC-Roof") {
        return 11;
    } else if (CodeName == "HHJ") {
        return 12;
    } else if (CodeName == "Gear-Desync") {
        return 13;
    } else if (CodeName == "Full-Gear-Desync") {
        return 14;
    } else if (CodeName == "Floor-Bounce-High-Jump") {
        return 15;
    } else if (CodeName == "Wallhop") {
        return 16;
    } else if (CodeName == "Wallwalk") {
        return 17;
    } else if (CodeName == "FPS-Drop") {
        return 18;
    } else return 2000;
}

inline void clearConsole() {
#ifdef _WIN32
    system("cls");
#else
    system("clear");
#endif
}

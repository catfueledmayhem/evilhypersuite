/* Hello!!

This is a simple, cross-platform, header-only library to suspend and resume processes written in C++.
It currently supports Windows and Linux.

Features:
- Suspend/resume a single process by PID
- Suspend/resume all processes by executable name
- Query if a process exists and can be controlled
- Find the parent PID or all descendants (process tree)
- Works on sandboxed applications on Linux (e.g., Snap/Flatpak)
- Automatically handles cgroups on Linux when possible
- Pure header-only: just include `procctrl.hpp` and use
- Optimized for fast batch operations

PERFORMANCE OPTIMIZATIONS:
- Cached NT function pointers (Windows)
- Single snapshot for batch operations (Windows)
- Reduced syscalls and file operations (Linux)
- Efficient process tree traversal
- Minimized string allocations

Usage example:
----------------------------------------------------------------------
#include "procctrl.hpp"

int main() {
    // Fast batch suspend/resume by name
    procctrl::suspend_processes_by_name("firefox");
    procctrl::resume_processes_by_name("firefox");

    // Single process control
    auto pid = procctrl::find_process_by_name("firefox");
    if (pid != -1) {
        procctrl::set_process_suspended(pid, true);  // suspend
        procctrl::set_process_suspended(pid, false); // resume
    }
}
----------------------------------------------------------------------
*/

#pragma once
#include <string>
#include <vector>
#include <unordered_set>
#include <unordered_map>
#include <fstream>
#include <sstream>
#include <algorithm>
#include <cstdio>
#include <cerrno>
#include <cstring>

#ifdef _WIN32
#define WIN32_LEAN_AND_MEAN
    #include <windows.h>
    #include <psapi.h>
    #include <tlhelp32.h>
    #ifndef _PID_T_
        typedef DWORD pid_t;
    #endif
#undef Rectangle
#undef CloseWindow
#undef ShowCursor
#else
    #include <csignal>
    #include <unistd.h>
    #include <dirent.h>
    #include <sys/stat.h>
#endif

#include "Helper.hpp"

namespace procctrl {

#ifdef _WIN32
// Windows implementation using undocumented NT API
typedef LONG(NTAPI *NtSuspendProcess)(HANDLE ProcessHandle);
typedef LONG(NTAPI *NtResumeProcess)(HANDLE ProcessHandle);

static NtSuspendProcess g_pfnNtSuspendProcess = nullptr;
static NtResumeProcess g_pfnNtResumeProcess = nullptr;
static bool g_initialized = false;

/// Enable SeDebugPrivilege to allow suspending processes owned by other users
/// @return true if privilege was enabled successfully
inline bool enable_debug_privilege() {
    HANDLE hToken;
    if (!OpenProcessToken(GetCurrentProcess(), TOKEN_ADJUST_PRIVILEGES | TOKEN_QUERY, &hToken)) {
        return false;
    }

    TOKEN_PRIVILEGES tp;
    LUID luid;

    if (!LookupPrivilegeValueA(nullptr, "SeDebugPrivilege", &luid)) {
        CloseHandle(hToken);
        return false;
    }

    tp.PrivilegeCount = 1;
    tp.Privileges[0].Luid = luid;
    tp.Privileges[0].Attributes = SE_PRIVILEGE_ENABLED;

    bool success = AdjustTokenPrivileges(hToken, FALSE, &tp, sizeof(TOKEN_PRIVILEGES), nullptr, nullptr) != 0;
    CloseHandle(hToken);
    return success;
}

inline void init_nt_functions() {
    if (!g_initialized) {
        HMODULE hNtdll = GetModuleHandleA("ntdll");
        if (hNtdll) {
            g_pfnNtSuspendProcess = reinterpret_cast<NtSuspendProcess>(
                GetProcAddress(hNtdll, "NtSuspendProcess"));
            g_pfnNtResumeProcess = reinterpret_cast<NtResumeProcess>(
                GetProcAddress(hNtdll, "NtResumeProcess"));
        }
        enable_debug_privilege();
        g_initialized = true;
    }
}

/// Internal optimized batch suspend/resume for Windows
/// Reuses a single snapshot for all operations
inline int batch_suspend_resume_windows(const std::string& exe_name, bool suspend) {
    init_nt_functions();

    if (!g_pfnNtSuspendProcess || !g_pfnNtResumeProcess) {
        log("[procctrl] Failed to load NT functions\n");
        return 0;
    }

    // Create snapshot once
    HANDLE hSnapshot = CreateToolhelp32Snapshot(TH32CS_SNAPPROCESS, 0);
    if (hSnapshot == INVALID_HANDLE_VALUE) {
        return 0;
    }

    PROCESSENTRY32W pe32;
    pe32.dwSize = sizeof(PROCESSENTRY32W);

    int success_count = 0;
    char szProcessName[MAX_PATH];

    if (Process32FirstW(hSnapshot, &pe32)) {
        do {
            // Convert wide to narrow
            WideCharToMultiByte(CP_UTF8, 0, pe32.szExeFile, -1,
                              szProcessName, MAX_PATH, nullptr, nullptr);

            if (exe_name == szProcessName) {
                HANDLE hProcess = OpenProcess(PROCESS_SUSPEND_RESUME, FALSE, pe32.th32ProcessID);
                if (hProcess) {
                    LONG status;
                    if (suspend) {
                        status = g_pfnNtSuspendProcess(hProcess);
                    } else {
                        status = g_pfnNtResumeProcess(hProcess);
                    }

                    if (status == 0) {
                        success_count++;
                    }

                    CloseHandle(hProcess);
                }
            }
        } while (Process32NextW(hSnapshot, &pe32));
    }

    CloseHandle(hSnapshot);
    return success_count;
}

#else
// Linux optimizations

/// Check if cgroup v2 is available on the system (cached)
inline bool is_cgroup_v2_available() {
    static int cached_result = -1;
    if (cached_result == -1) {
        struct stat st;
        cached_result = (stat("/sys/fs/cgroup/cgroup.controllers", &st) == 0) ? 1 : 0;
    }
    return cached_result == 1;
}

/// Get the cgroup v2 filesystem path of a process (optimized)
inline std::string get_cgroup_v2_path(pid_t pid) {
    char path_buf[64];
    snprintf(path_buf, sizeof(path_buf), "/proc/%d/cgroup", static_cast<int>(pid));

    std::ifstream cgroup_file(path_buf);
    if (!cgroup_file) return "";

    std::string line;
    line.reserve(256);

    while (std::getline(cgroup_file, line)) {
        if (line.length() > 4 && line[0] == '0' && line[1] == ':' && line[2] == ':' && line[3] == '/') {
            std::string result = "/sys/fs/cgroup";
            result.append(line.begin() + 3, line.end());
            return result;
        }
    }
    return "";
}

/// Optimized batch operation for Linux - single /proc scan
inline int batch_suspend_resume_linux(const std::string& exe_name, bool suspend) {
    DIR* proc_dir = opendir("/proc");
    if (!proc_dir) {
        log("[procctrl] Failed to open /proc");
        return 0;
    }

    std::unordered_set<std::string> handled_cgroups;
    int success_count = 0;
    const int signal_to_send = suspend ? SIGSTOP : SIGCONT;
    const char* action_signal = suspend ? "SIGSTOP" : "SIGCONT";
    const char* action_cgroup = suspend ? "Freezing" : "Thawing";
    const bool cgroup_available = is_cgroup_v2_available();

    struct dirent* entry;
    char comm_path[64];
    char comm_buf[256];

    while ((entry = readdir(proc_dir)) != nullptr) {
        // Quick checks for valid PID
        if (entry->d_type != DT_UNKNOWN && entry->d_type != DT_DIR) continue;
        if (entry->d_name[0] < '1' || entry->d_name[0] > '9') continue;

        char* endptr;
        pid_t pid = strtol(entry->d_name, &endptr, 10);
        if (*endptr != '\0') continue;

        // Read process name efficiently
        snprintf(comm_path, sizeof(comm_path), "/proc/%s/comm", entry->d_name);
        FILE* comm_file = fopen(comm_path, "r");
        if (!comm_file) continue;

        if (fgets(comm_buf, sizeof(comm_buf), comm_file)) {
            // Remove trailing newline
            size_t len = strlen(comm_buf);
            if (len > 0 && comm_buf[len - 1] == '\n') {
                comm_buf[len - 1] = '\0';
            }

            if (exe_name == comm_buf) {
                std::string cgroup_path = get_cgroup_v2_path(pid);

                bool is_sandboxed = !cgroup_path.empty() &&
                                   (cgroup_path.find("app-") != std::string::npos ||
                                    cgroup_path.find("snap.") != std::string::npos);

                // Skip if we already handled this cgroup
                if (is_sandboxed && !cgroup_path.empty()) {
                    if (handled_cgroups.count(cgroup_path)) {
                        fclose(comm_file);
                        continue;
                    }
                    handled_cgroups.insert(cgroup_path);
                }

                // Handle sandboxed app with cgroup
                if (is_sandboxed && cgroup_available) {
                    std::string freeze_file_path = cgroup_path + "/cgroup.freeze";
                    FILE* freeze_file = fopen(freeze_file_path.c_str(), "w");

                    if (freeze_file) {
                        fprintf(freeze_file, "%d", suspend ? 1 : 0);
                        fclose(freeze_file);
                        success_count++;
                    }
                } else {
                    // Standard signal-based suspend/resume
                    if (kill(pid, signal_to_send) == 0) {
                        success_count++;
                    }
                }
            }
        }

        fclose(comm_file);
    }

    closedir(proc_dir);
    return success_count;
}
#endif

/// Check if a process still exists (optimized)
inline bool process_exists(pid_t pid) {
#ifdef _WIN32
    HANDLE hProcess = OpenProcess(PROCESS_QUERY_LIMITED_INFORMATION, FALSE, pid);
    if (hProcess) {
        CloseHandle(hProcess);
        return true;
    }
    return false;
#else
    return kill(pid, 0) == 0 || errno == EPERM;
#endif
}

/// Check if the current user can control a process
inline bool can_control_process(pid_t pid) {
#ifdef _WIN32
    HANDLE hProcess = OpenProcess(PROCESS_SUSPEND_RESUME, FALSE, pid);
    if (hProcess) {
        CloseHandle(hProcess);
        return true;
    }
    return false;
#else
    return kill(pid, 0) == 0 || errno == EPERM;
#endif
}

/// Find the first process ID by executable name (optimized)
inline pid_t find_process_by_name(const std::string& exe_name) {
#ifdef _WIN32
    HANDLE hSnapshot = CreateToolhelp32Snapshot(TH32CS_SNAPPROCESS, 0);
    if (hSnapshot == INVALID_HANDLE_VALUE) {
        return static_cast<pid_t>(-1);
    }

    PROCESSENTRY32W pe32;
    pe32.dwSize = sizeof(PROCESSENTRY32W);
    char szProcessName[MAX_PATH];

    if (Process32FirstW(hSnapshot, &pe32)) {
        do {
            WideCharToMultiByte(CP_UTF8, 0, pe32.szExeFile, -1,
                              szProcessName, MAX_PATH, nullptr, nullptr);

            if (exe_name == szProcessName) {
                CloseHandle(hSnapshot);
                return pe32.th32ProcessID;
            }
        } while (Process32NextW(hSnapshot, &pe32));
    }

    CloseHandle(hSnapshot);
    return static_cast<pid_t>(-1);
#else
    DIR* proc_dir = opendir("/proc");
    if (!proc_dir) {
        return -1;
    }

    struct dirent* entry;
    pid_t result = -1;
    char comm_path[64];
    char comm_buf[256];

    while ((entry = readdir(proc_dir)) != nullptr) {
        if (entry->d_type != DT_UNKNOWN && entry->d_type != DT_DIR) continue;
        if (entry->d_name[0] < '1' || entry->d_name[0] > '9') continue;

        char* endptr;
        pid_t pid = strtol(entry->d_name, &endptr, 10);
        if (*endptr != '\0') continue;

        snprintf(comm_path, sizeof(comm_path), "/proc/%s/comm", entry->d_name);
        FILE* comm_file = fopen(comm_path, "r");
        if (!comm_file) continue;

        if (fgets(comm_buf, sizeof(comm_buf), comm_file)) {
            size_t len = strlen(comm_buf);
            if (len > 0 && comm_buf[len - 1] == '\n') {
                comm_buf[len - 1] = '\0';
            }

            if (exe_name == comm_buf) {
                result = pid;
                fclose(comm_file);
                break;
            }
        }

        fclose(comm_file);
    }

    closedir(proc_dir);
    return result;
#endif
}

/// Find all process IDs by executable name (optimized)
inline std::vector<pid_t> find_all_processes_by_name(const std::string& exe_name) {
    std::vector<pid_t> pids;
    pids.reserve(16); // Pre-allocate for common case

#ifdef _WIN32
    HANDLE hSnapshot = CreateToolhelp32Snapshot(TH32CS_SNAPPROCESS, 0);
    if (hSnapshot == INVALID_HANDLE_VALUE) {
        return pids;
    }

    PROCESSENTRY32W pe32;
    pe32.dwSize = sizeof(PROCESSENTRY32W);
    char szProcessName[MAX_PATH];

    if (Process32FirstW(hSnapshot, &pe32)) {
        do {
            WideCharToMultiByte(CP_UTF8, 0, pe32.szExeFile, -1,
                              szProcessName, MAX_PATH, nullptr, nullptr);

            if (exe_name == szProcessName) {
                pids.push_back(pe32.th32ProcessID);
            }
        } while (Process32NextW(hSnapshot, &pe32));
    }

    CloseHandle(hSnapshot);
#else
    DIR* proc_dir = opendir("/proc");
    if (!proc_dir) {
        return pids;
    }

    struct dirent* entry;
    char comm_path[64];
    char comm_buf[256];

    while ((entry = readdir(proc_dir)) != nullptr) {
        if (entry->d_type != DT_UNKNOWN && entry->d_type != DT_DIR) continue;
        if (entry->d_name[0] < '1' || entry->d_name[0] > '9') continue;

        char* endptr;
        pid_t pid = strtol(entry->d_name, &endptr, 10);
        if (*endptr != '\0') continue;

        snprintf(comm_path, sizeof(comm_path), "/proc/%s/comm", entry->d_name);
        FILE* comm_file = fopen(comm_path, "r");
        if (!comm_file) continue;

        if (fgets(comm_buf, sizeof(comm_buf), comm_file)) {
            size_t len = strlen(comm_buf);
            if (len > 0 && comm_buf[len - 1] == '\n') {
                comm_buf[len - 1] = '\0';
            }

            if (exe_name == comm_buf) {
                pids.push_back(pid);
            }
        }

        fclose(comm_file);
    }

    closedir(proc_dir);
#endif

    return pids;
}

/// Get the parent process ID (PPID) of a given process (optimized)
inline pid_t get_parent_pid(pid_t pid) {
#ifdef _WIN32
    HANDLE hSnapshot = CreateToolhelp32Snapshot(TH32CS_SNAPPROCESS, 0);
    if (hSnapshot == INVALID_HANDLE_VALUE) {
        return static_cast<pid_t>(-1);
    }

    PROCESSENTRY32W pe32;
    pe32.dwSize = sizeof(PROCESSENTRY32W);
    pid_t ppid = static_cast<pid_t>(-1);

    if (Process32FirstW(hSnapshot, &pe32)) {
        do {
            if (pe32.th32ProcessID == pid) {
                ppid = pe32.th32ParentProcessID;
                break;
            }
        } while (Process32NextW(hSnapshot, &pe32));
    }

    CloseHandle(hSnapshot);
    return ppid;
#else
    char stat_path[64];
    snprintf(stat_path, sizeof(stat_path), "/proc/%d/stat", static_cast<int>(pid));

    FILE* stat_file = fopen(stat_path, "r");
    if (!stat_file) return -1;

    char line[512];
    if (!fgets(line, sizeof(line), stat_file)) {
        fclose(stat_file);
        return -1;
    }
    fclose(stat_file);

    // Find the last ')' to skip process name
    char* pos = strrchr(line, ')');
    if (!pos) return -1;

    char state;
    pid_t ppid;
    if (sscanf(pos + 1, " %c %d", &state, &ppid) == 2) {
        return ppid;
    }

    return -1;
#endif
}

/// Suspend or resume a process (optimized for single operations)
inline bool set_process_suspended(pid_t pid, bool suspend) {
    if (!process_exists(pid)) {
        return false;
    }

#ifdef _WIN32
    init_nt_functions();

    if (!g_pfnNtSuspendProcess || !g_pfnNtResumeProcess) {
        return false;
    }

    HANDLE hProcess = OpenProcess(PROCESS_SUSPEND_RESUME, FALSE, pid);
    if (!hProcess) {
        return false;
    }

    LONG status = suspend ? g_pfnNtSuspendProcess(hProcess) : g_pfnNtResumeProcess(hProcess);
    CloseHandle(hProcess);
    return status == 0;
#else
    std::string cgroup_path = get_cgroup_v2_path(pid);

    bool is_sandboxed = !cgroup_path.empty() &&
                       (cgroup_path.find("app-") != std::string::npos ||
                        cgroup_path.find("snap.") != std::string::npos);

    if (is_sandboxed && is_cgroup_v2_available()) {
        std::string freeze_file_path = cgroup_path + "/cgroup.freeze";
        FILE* freeze_file = fopen(freeze_file_path.c_str(), "w");

        if (freeze_file) {
            fprintf(freeze_file, "%d", suspend ? 1 : 0);
            fclose(freeze_file);
            return true;
        }
        return false;
    } else {
        int signal_to_send = suspend ? SIGSTOP : SIGCONT;
        return kill(pid, signal_to_send) == 0;
    }
#endif
}

/// Suspend all processes by executable name (OPTIMIZED FOR BATCH)
/// @param exe_name Name of the executable
/// @return Number of processes/cgroups successfully suspended
inline int suspend_processes_by_name(const std::string& exe_name) {
#ifdef _WIN32
    return batch_suspend_resume_windows(exe_name, true);
#else
    return batch_suspend_resume_linux(exe_name, true);
#endif
}

/// Resume all processes by executable name (OPTIMIZED FOR BATCH)
/// @param exe_name Name of the executable
/// @return Number of processes/cgroups successfully resumed
inline int resume_processes_by_name(const std::string& exe_name) {
#ifdef _WIN32
    return batch_suspend_resume_windows(exe_name, false);
#else
    return batch_suspend_resume_linux(exe_name, false);
#endif
}

/// Get all PIDs in a process tree (parent and all descendants) - optimized
inline std::vector<pid_t> get_process_tree(pid_t root_pid) {
    std::vector<pid_t> tree;
    tree.reserve(32);
    tree.push_back(root_pid);

    std::vector<pid_t> to_check;
    to_check.reserve(32);
    to_check.push_back(root_pid);

#ifdef _WIN32
    // Single snapshot for entire tree
    HANDLE hSnapshot = CreateToolhelp32Snapshot(TH32CS_SNAPPROCESS, 0);
    if (hSnapshot == INVALID_HANDLE_VALUE) return tree;

    while (!to_check.empty()) {
        pid_t current = to_check.back();
        to_check.pop_back();

        PROCESSENTRY32W pe32;
        pe32.dwSize = sizeof(PROCESSENTRY32W);

        if (Process32FirstW(hSnapshot, &pe32)) {
            do {
                if (pe32.th32ParentProcessID == current) {
                    tree.push_back(pe32.th32ProcessID);
                    to_check.push_back(pe32.th32ProcessID);
                }
            } while (Process32NextW(hSnapshot, &pe32));
        }
    }

    CloseHandle(hSnapshot);
#else
    while (!to_check.empty()) {
        pid_t current = to_check.back();
        to_check.pop_back();

        DIR* proc_dir = opendir("/proc");
        if (!proc_dir) continue;

        struct dirent* entry;
        while ((entry = readdir(proc_dir)) != nullptr) {
            if (entry->d_type != DT_UNKNOWN && entry->d_type != DT_DIR) continue;
            if (entry->d_name[0] < '1' || entry->d_name[0] > '9') continue;

            char* endptr;
            pid_t pid = strtol(entry->d_name, &endptr, 10);
            if (*endptr != '\0') continue;

            if (get_parent_pid(pid) == current) {
                tree.push_back(pid);
                to_check.push_back(pid);
            }
        }
        closedir(proc_dir);
    }
#endif

    return tree;
}

} // namespace procctrl

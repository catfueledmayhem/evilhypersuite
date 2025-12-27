/*
===============================================================================
NetCtrl - Cross-Platform Network Control Library (Header-Only)
===============================================================================

Author: 3443
Date: [22/11/2025]
License: MIT

Description:
-------------
NetCtrl is a lightweight, header-only C++ library for programmatically
controlling network traffic. Uses WinDivert on Windows and tc netem on Linux.

Features:
----------
- Block network traffic completely
- Increase ping/latency (RTT)
- Apply packet loss
- Combine ping increase + packet loss
- Filter by specific IP addresses
- Firewall rules for blocking when prevent_disconnect is false
- Clean removal of all rules
- Header-only, cross-platform
- Fast packet processing with optimized threading
- Silent execution (no command prompt windows on Windows)

Windows Requirements:
---------------------
- WinDivert64.dll or WinDivert32.dll must be in the same directory or system PATH
- WinDivert driver files (WinDivert64.sys / WinDivert32.sys)
- Requires administrator privileges
- WinDivert: https://www.reqrypt.org/windivert.html

Linux Requirements:
-------------------
- tc (traffic control) utility (iproute2 package)
- iptables
- Requires root privileges

Building on Linux for Windows (MinGW):
---------------------------------------
1. Install MinGW: sudo apt-get install mingw-w64
2. Download WinDivert SDK from https://www.reqrypt.org/windivert.html
3. Compile:
   x86_64-w64-mingw32-g++ -o netctrl.exe main.cpp \
       -I/path/to/WinDivert/include \
       -L/path/to/WinDivert/x64 \
       -lWinDivert -lws2_32 -static-libgcc -static-libstdc++ -std=c++11

Usage Example (C++):
---------------------
#include "netctrl.hpp"
#include <iostream>

int main() {
    netctrl::NetCtrl net;

    if (!netctrl::NetCtrl::isAdmin()) {
        std::cerr << "Run as administrator/root!" << std::endl;
        return 1;
    }

    // Filter specific IP
    net.setTargetIP("192.168.1.100");

    // Increase ping by 200ms
    net.increasePing(200);

    // Or combine ping increase + packet loss
    net.lag(150, 5.0);  // 150ms delay + 5% packet loss

    // Block all traffic
    net.block();

    // Remove all controls
    net.disable();

    return 0;
}

===============================================================================
*/

#ifndef NETCTRL_HPP
#define NETCTRL_HPP

#include <string>
#include <vector>
#include <cstdio>
#include <cstdlib>
#include <sstream>
#include <iomanip>
#include <iostream>
#include <cstring>
#include <random>
#include <chrono>
#include <thread>
#include <atomic>
#include <queue>
#include <mutex>
#include <memory>

#ifdef _WIN32
#ifndef NETCTRL_WINDOWS_INCLUDED
#define NETCTRL_WINDOWS_INCLUDED
#define WIN32_LEAN_AND_MEAN

#ifndef NOMINMAX
#define NOMINMAX
#endif

// Include WinDivert BEFORE other headers to avoid macro conflicts
#include "windivert.h"

// Rename Windows functions to avoid conflicts with Raylib
#define Rectangle Win32Rectangle
#define CloseWindow Win32CloseWindow
#define ShowCursor Win32ShowCursor
#define DrawText Win32DrawText
#define DrawTextEx Win32DrawTextEx
#define LoadImage Win32LoadImage

#include <windows.h>
#include <winsock2.h>
#include <ws2tcpip.h>

// Restore original names after Windows header is included
#undef Rectangle
#undef CloseWindow
#undef ShowCursor
#undef DrawText
#undef DrawTextEx
#undef LoadImage

// Undefine SAL annotations that conflict with STL
#ifdef __out
#undef __out
#endif
#ifdef __in
#undef __in
#endif
#ifdef __inout
#undef __inout
#endif

#endif
#else
#include <unistd.h>
#endif

namespace netctrl {

enum class Direction {
    Inbound,
    Outbound,
    Both
};

class NetCtrl {
public:
    NetCtrl() : prevent_disconnect_(true) {
        findInterface();
    }

    ~NetCtrl() {
        disable();
    }

    // ========================================================================
    // PRIMARY API METHODS
    // ========================================================================

    /**
     * Set whether to prevent disconnection (use WinDivert) or allow it (use firewall)
     * @param prevent If true, uses WinDivert (keeps connection). If false, uses firewall (disconnects)
     */
    void setPreventDisconnect(bool prevent) {
        if (prevent_disconnect_ != prevent) {
            prevent_disconnect_ = prevent;
            std::cout << "[INFO] Prevent disconnect set to: " << (prevent ? "true" : "false") << std::endl;

            // If currently active, restart with new method
            if (is_active_) {
                int saved_lag = current_lag_ms_;
                double saved_drop = current_drop_percent_;
                disable();
                lag(saved_lag, saved_drop);
            }
        }
    }

    /**
     * Set a specific IP address to filter
     * @param ip Target IP address (e.g., "192.168.1.100"), empty string for all traffic
     */
    void setTargetIP(const std::string& ip) {
        target_ip_ = ip;
        if (ip.empty()) {
            std::cout << "[INFO] Target IP cleared - affecting all traffic" << std::endl;
        } else {
            std::cout << "[INFO] Target IP set to: " << ip << std::endl;
        }
    }

    /**
     * Clear target IP filter (affect all traffic)
     */
    void clearTargetIP() {
        target_ip_.clear();
        std::cout << "[INFO] Target IP filter cleared" << std::endl;
    }

    /**
     * Increase ping/latency by specified milliseconds
     * @param ms Milliseconds to add to network latency (RTT will increase by ~2x this)
     * @return true if successful
     */
    bool increasePing(int ms) {
        if (ms <= 0) {
            std::cerr << "[ERROR] Ping increase must be > 0" << std::endl;
            return false;
        }
        std::cout << "[INFO] Increasing ping by " << ms << "ms" << std::endl;
        return lag(ms, 0.0);
    }

    /**
     * Apply network lag with optional packet loss
     * @param lag_ms Delay in milliseconds (increases ping/RTT)
     * @param drop_percent Packet loss percentage (0-100)
     * @return true if successful
     */
    bool lag(int lag_ms, double drop_percent) {
        std::cout << "[INFO] Applying lag=" << lag_ms << "ms, loss=" << drop_percent << "%" << std::endl;
#ifdef _WIN32
        return applyWindowsLag(lag_ms, drop_percent);
#else
        return applyLinux(lag_ms, drop_percent);
#endif
    }

    /**
     * Block all network traffic completely (100% packet loss)
     * @return true if successful
     */
    bool block() {
        std::cout << "[INFO] Blocking all network traffic..." << std::endl;
#ifdef _WIN32
        return applyWindowsLag(0, 100.0);
#else
        return blockLinux();
#endif
    }

    /**
     * Remove all network controls and restore normal operation
     * @return true if successful
     */
    bool disable() {
        std::cout << "[INFO] Disabling all network controls..." << std::endl;
#ifdef _WIN32
        return disableWindows();
#else
        return disableLinux();
#endif
    }

    // ========================================================================
    // STATUS METHODS
    // ========================================================================

    bool isActive() const { return is_active_; }
    int getLag() const { return current_lag_ms_; }
    double getDrop() const { return current_drop_percent_; }
    std::string getTargetIP() const { return target_ip_; }
    bool getPreventDisconnect() const { return prevent_disconnect_; }

    static bool isAdmin() {
#ifdef _WIN32
        BOOL admin = FALSE;
        PSID grp = NULL;
        SID_IDENTIFIER_AUTHORITY auth = SECURITY_NT_AUTHORITY;
        if (AllocateAndInitializeSid(&auth, 2, SECURITY_BUILTIN_DOMAIN_RID,
                                      DOMAIN_ALIAS_RID_ADMINS, 0, 0, 0, 0, 0, 0, &grp)) {
            CheckTokenMembership(NULL, grp, &admin);
            FreeSid(grp);
        }
        return admin;
#else
        return geteuid() == 0;
#endif
    }

private:
    bool is_active_ = false;
    int current_lag_ms_ = 0;
    double current_drop_percent_ = 0.0;
    std::string default_iface_;
    std::string target_ip_;
    bool prevent_disconnect_ = true;

#ifdef _WIN32
    std::atomic<bool> stop_thread_{false};
    std::thread divert_thread_;
    HANDLE divert_handle_ = INVALID_HANDLE_VALUE;

    struct PacketInfo {
        std::vector<uint8_t> data;
        WINDIVERT_ADDRESS addr;
        std::chrono::steady_clock::time_point release_time;
    };

    // Silent system command execution (no CMD window)
    static int execSilent(const std::string& command) {
        STARTUPINFOA si = {sizeof(si)};
        PROCESS_INFORMATION pi = {0};

        si.dwFlags = STARTF_USESHOWWINDOW;
        si.wShowWindow = SW_HIDE;  // Hide the window

        std::string cmdLine = "cmd.exe /C " + command;
        std::vector<char> cmdLineBuffer(cmdLine.begin(), cmdLine.end());
        cmdLineBuffer.push_back('\0');

        if (!CreateProcessA(NULL, cmdLineBuffer.data(), NULL, NULL, FALSE,
                           CREATE_NO_WINDOW, NULL, NULL, &si, &pi)) {
            return -1;
        }

        WaitForSingleObject(pi.hProcess, INFINITE);

        DWORD exitCode = 0;
        GetExitCodeProcess(pi.hProcess, &exitCode);

        CloseHandle(pi.hProcess);
        CloseHandle(pi.hThread);

        return exitCode;
    }
#endif

    void findInterface() {
#ifndef _WIN32
        FILE* p = popen("ip route show default | awk '/default/ {print $5}' | head -1", "r");
        if (p) {
            char buf[64];
            if (fgets(buf, sizeof(buf), p)) {
                default_iface_ = buf;
                if (!default_iface_.empty() && default_iface_.back() == '\n') {
                    default_iface_.pop_back();
                }
            }
            pclose(p);
        }

        if (default_iface_.empty()) {
            std::vector<std::string> common = {"eth0", "eno1", "enp0s3", "wlan0", "wlp2s0"};
            for (const auto& iface : common) {
                std::string check = "ip link show " + iface + " 2>/dev/null";
                if (system(check.c_str()) == 0) {
                    default_iface_ = iface;
                    break;
                }
            }
        }

        std::cout << "[DEBUG] Using interface: " << default_iface_ << std::endl;
#endif
    }

#ifdef _WIN32
    static void packetHandlerThread(HANDLE handle, int lag_ms, double drop_percent,
                                   std::atomic<bool>* stop_flag, uint32_t target_ip) {
        if (!handle || handle == INVALID_HANDLE_VALUE) {
            fprintf(stderr, "[NetCtrl] Invalid handle in thread\n");
            return;
        }

        // Pre-allocate packet buffer
        std::vector<uint8_t> packet_buffer(65535);
        WINDIVERT_ADDRESS addr;
        UINT packet_len;

        // RNG for packet loss
        std::random_device rd;
        std::mt19937 gen(rd());
        std::uniform_real_distribution<> dis(0.0, 100.0);

        // Delay queue with pre-allocated capacity
        std::vector<PacketInfo> delayed_packets;
        delayed_packets.reserve(1024);

        std::cout << "[NetCtrl] Packet handler thread started" << std::endl;

        const bool has_target = (target_ip != 0);
        const bool has_delay = (lag_ms > 0);
        const bool has_drop = (drop_percent > 0);

        while (!stop_flag->load()) {
            // Non-blocking receive with timeout
            if (!WinDivertRecv(handle, packet_buffer.data(), (UINT)packet_buffer.size(),
                              &packet_len, &addr)) {
                DWORD err = GetLastError();
                if (err == ERROR_NO_DATA || err == ERROR_OPERATION_ABORTED) {
                    // Normal shutdown or no data
                    std::this_thread::sleep_for(std::chrono::microseconds(100));
                    continue;
                }
                if (err != ERROR_INVALID_HANDLE) {
                    fprintf(stderr, "[NetCtrl] Receive error: %lu\n", err);
                }
                break;
            }

            // Fast path: check if packet matches target IP filter
            if (has_target && packet_len > 0) {
                PWINDIVERT_IPHDR ip_header = nullptr;
                PWINDIVERT_IPV6HDR ipv6_header = nullptr;
                UINT8 protocol = 0;
                PWINDIVERT_ICMPHDR icmp_header = nullptr;
                PWINDIVERT_ICMPV6HDR icmpv6_header = nullptr;
                PWINDIVERT_TCPHDR tcp_header = nullptr;
                PWINDIVERT_UDPHDR udp_header = nullptr;
                PVOID payload = nullptr;
                UINT payload_len = 0;
                PVOID reserved1 = nullptr;
                UINT reserved2 = 0;

                if (WinDivertHelperParsePacket(packet_buffer.data(), packet_len,
                                          &ip_header, &ipv6_header, &protocol,
                                          &icmp_header, &icmpv6_header,
                                          &tcp_header, &udp_header,
                                          &payload, &payload_len, &reserved1, &reserved2)) {

                    if (ip_header && ntohl(ip_header->DstAddr) != target_ip) {
                        // Not our target, pass through immediately
                        WinDivertSend(handle, packet_buffer.data(), packet_len, nullptr, &addr);
                        continue;
                    }
                }
            }

            // Check for packet drop
            if (has_drop) {
                if (drop_percent >= 100.0 || dis(gen) < drop_percent) {
                    // Drop packet
                    continue;
                }
            }

            // Handle delay
            if (has_delay) {
                PacketInfo pi;
                pi.data.assign(packet_buffer.begin(), packet_buffer.begin() + packet_len);
                pi.addr = addr;
                pi.release_time = std::chrono::steady_clock::now() +
                                 std::chrono::milliseconds(lag_ms);
                delayed_packets.push_back(std::move(pi));
            } else {
                // No delay - send immediately
                WinDivertSend(handle, packet_buffer.data(), packet_len, nullptr, &addr);
            }

            // Process delayed packets efficiently
            if (!delayed_packets.empty()) {
                auto now = std::chrono::steady_clock::now();
                size_t sent_count = 0;

                for (size_t i = 0; i < delayed_packets.size(); ++i) {
                    if (now >= delayed_packets[i].release_time) {
                        WinDivertSend(handle, delayed_packets[i].data.data(),
                                    (UINT)delayed_packets[i].data.size(),
                                    nullptr, &delayed_packets[i].addr);
                        sent_count++;
                    } else if (sent_count > 0) {
                        // Move packet forward in queue
                        delayed_packets[i - sent_count] = std::move(delayed_packets[i]);
                    }
                }

                // Remove sent packets
                if (sent_count > 0) {
                    delayed_packets.resize(delayed_packets.size() - sent_count);
                }
            }
        }

        // Flush remaining delayed packets
        for (auto& pi : delayed_packets) {
            WinDivertSend(handle, pi.data.data(), (UINT)pi.data.size(), nullptr, &pi.addr);
        }

        std::cout << "[NetCtrl] Packet handler thread stopped" << std::endl;
    }

    std::string buildFilter() {
        std::stringstream filter;

        // If no target IP, affect all traffic
        if (target_ip_.empty()) {
            filter << "udp or tcp";  // All UDP and TCP traffic
        } else {
            // Base filter for UDP on high ports (games typically use these)
            filter << "(udp and udp.DstPort >= 49152)";

            // Add IP filter if specified
            filter << " and ip.DstAddr == " << target_ip_;
        }

        return filter.str();
    }

    uint32_t ipStringToInt(const std::string& ip) {
        if (ip.empty()) return 0;

        struct in_addr addr;
        if (inet_pton(AF_INET, ip.c_str(), &addr) == 1) {
            return ntohl(addr.s_addr);
        }
        return 0;
    }

    bool addFirewallRule(const std::string& rule_name, const std::string& ip, bool block_out, bool block_in) {
        std::string direction = block_out ? "out" : "in";
        std::stringstream cmd;

        if (ip.empty()) {
            // Block all traffic
            cmd << "netsh advfirewall firewall add rule name=\"" << rule_name
                << "\" dir=" << direction << " action=block >nul 2>&1";
        } else {
            // Block specific IP
            cmd << "netsh advfirewall firewall add rule name=\"" << rule_name
                << "\" dir=" << direction << " action=block remoteip=" << ip << " >nul 2>&1";
        }

        return execSilent(cmd.str()) == 0;
    }

    bool removeFirewallRule(const std::string& rule_name) {
        std::string cmd = "netsh advfirewall firewall delete rule name=\"" + rule_name + "\" >nul 2>&1";
        return execSilent(cmd) == 0;
    }

    bool applyWindowsLag(int lag_ms, double drop_percent) {
        // Stop any existing divert operations
        disableWindows();

        // Validate parameters
        if (lag_ms < 0) lag_ms = 0;
        if (drop_percent < 0) drop_percent = 0;
        if (drop_percent > 100) drop_percent = 100;

        // If prevent_disconnect is false and we're blocking, use firewall instead
        if (!prevent_disconnect_ && drop_percent >= 100.0) {
            std::cout << "[INFO] Using firewall rules (will disconnect)" << std::endl;

            std::string rule_base = "NetCtrl_Block";
            bool success = true;

            // Add outbound and inbound rules
            success &= addFirewallRule(rule_base + "_Out", target_ip_, true, false);
            success &= addFirewallRule(rule_base + "_In", target_ip_, false, true);

            if (success) {
                is_active_ = true;
                current_lag_ms_ = 0;
                current_drop_percent_ = 100.0;
                std::cout << "[SUCCESS] Firewall rules applied" << std::endl;
                return true;
            } else {
                std::cerr << "[ERROR] Failed to apply firewall rules" << std::endl;
                return false;
            }
        }

        // Use WinDivert for lag/partial packet loss (prevents disconnect)
        std::string filter = buildFilter();

        std::cout << "[DEBUG] Opening WinDivert with filter: " << filter << std::endl;

        // Open WinDivert handle with low priority for faster startup
        divert_handle_ = WinDivertOpen(filter.c_str(), WINDIVERT_LAYER_NETWORK, -1000, 0);

        if (divert_handle_ == INVALID_HANDLE_VALUE) {
            DWORD error = GetLastError();
            std::cerr << "[ERROR] Failed to open WinDivert handle. Error: " << error << std::endl;

            if (error == ERROR_FILE_NOT_FOUND) {
                std::cerr << "[INFO] WinDivert driver not found. Please ensure WinDivert.dll and .sys files are present." << std::endl;
            } else if (error == ERROR_ACCESS_DENIED) {
                std::cerr << "[INFO] Access denied. Please run as administrator." << std::endl;
            } else if (error == ERROR_INVALID_PARAMETER) {
                std::cerr << "[INFO] Invalid filter syntax: " << filter << std::endl;
            }

            return false;
        }

        // Optimize WinDivert parameters for performance
        if (!WinDivertSetParam(divert_handle_, WINDIVERT_PARAM_QUEUE_LENGTH, 8192) ||
            !WinDivertSetParam(divert_handle_, WINDIVERT_PARAM_QUEUE_TIME, 2048)) {
            std::cerr << "[WARNING] Failed to set WinDivert parameters" << std::endl;
        }

        std::cout << "[SUCCESS] WinDivert handle opened successfully!" << std::endl;

        // Convert target IP to network byte order
        uint32_t target_ip_int = ipStringToInt(target_ip_);

        // Start packet handling thread with higher priority
        stop_thread_.store(false);

        try {
            divert_thread_ = std::thread(packetHandlerThread, divert_handle_,
                                         lag_ms, drop_percent, &stop_thread_, target_ip_int);

            // Set thread priority to time critical for minimal latency
            HANDLE thread_handle = (HANDLE)(intptr_t)divert_thread_.native_handle();
            if (thread_handle && thread_handle != INVALID_HANDLE_VALUE) {
                SetThreadPriority(thread_handle, THREAD_PRIORITY_TIME_CRITICAL);
            }
        } catch (const std::exception& e) {
            std::cerr << "[ERROR] Failed to start packet handler thread: " << e.what() << std::endl;
            WinDivertClose(divert_handle_);
            divert_handle_ = INVALID_HANDLE_VALUE;
            return false;
        }

        is_active_ = true;
        current_lag_ms_ = lag_ms;
        current_drop_percent_ = drop_percent;

        if (!target_ip_.empty()) {
            std::cout << "  → Target IP: " << target_ip_ << std::endl;
        } else {
            std::cout << "  → Target: All traffic" << std::endl;
        }
        if (lag_ms > 0) {
            std::cout << "  → Delay: " << lag_ms << "ms" << std::endl;
        }
        if (drop_percent > 0) {
            if (drop_percent >= 100.0) {
                std::cout << "  → Traffic blocked (100% packet loss)" << std::endl;
            } else {
                std::cout << "  → Packet loss: " << drop_percent << "%" << std::endl;
            }
        }

        return true;
    }

    bool disableWindows() {
        bool success = true;

        // Stop WinDivert thread
        if (divert_thread_.joinable()) {
            std::cout << "[DEBUG] Stopping WinDivert thread..." << std::endl;
            stop_thread_.store(true);

            // Give thread time to finish processing (with timeout)
            if (divert_thread_.joinable()) {
                divert_thread_.join();
            }
        }

        if (divert_handle_ != INVALID_HANDLE_VALUE) {
            std::cout << "[DEBUG] Closing WinDivert handle..." << std::endl;
            WinDivertClose(divert_handle_);
            divert_handle_ = INVALID_HANDLE_VALUE;
        }

        // Remove firewall rules if they exist
        removeFirewallRule("NetCtrl_Block_Out");
        removeFirewallRule("NetCtrl_Block_In");

        is_active_ = false;
        current_lag_ms_ = 0;
        current_drop_percent_ = 0.0;

        std::cout << "[SUCCESS] Network controls disabled" << std::endl;
        return success;
    }
#else
    bool blockLinux() {
        disableLinux();

        std::cout << "[DEBUG] Adding iptables DROP rules..." << std::endl;

        if (!target_ip_.empty()) {
            std::string cmd = "iptables -w -I OUTPUT 1 -d " + target_ip_ + " -j DROP &";
            system(cmd.c_str());
        } else {
            system("iptables -w -I OUTPUT 1 -j DROP &");
            system("iptables -w -I INPUT 1 -j DROP &");
        }

        is_active_ = true;
        current_lag_ms_ = 0;
        current_drop_percent_ = 100.0;
        return true;
    }

    bool applyLinux(int lag_ms, double drop_percent) {
        disableLinux();

        if (default_iface_.empty()) {
            std::cerr << "[ERROR] No network interface found!" << std::endl;
            return false;
        }

        std::cout << "[DEBUG] Applying tc netem on " << default_iface_ << std::endl;

        if (!target_ip_.empty()) {
            // Use tc with filters for IP-specific control
            std::stringstream setup_cmd;
            setup_cmd << "tc qdisc add dev " << default_iface_ << " root handle 1: prio 2>/dev/null";
            system(setup_cmd.str().c_str());

            // Add netem qdisc to band 1
            std::stringstream netem_cmd;
            netem_cmd << "tc qdisc add dev " << default_iface_ << " parent 1:1 handle 10: netem";

            if (lag_ms > 0) {
                netem_cmd << " delay " << lag_ms << "ms";
            }

            if (drop_percent > 0 && drop_percent < 100) {
                netem_cmd << " loss " << std::fixed << std::setprecision(2) << drop_percent << "%";
            } else if (drop_percent >= 100) {
                netem_cmd << " loss 100%";
            }

            netem_cmd << " 2>/dev/null";
            std::cout << "[DEBUG] Executing: " << netem_cmd.str() << std::endl;
            system(netem_cmd.str().c_str());

            // Add filter to route target IP traffic through netem
            std::stringstream filter_cmd;
            filter_cmd << "tc filter add dev " << default_iface_
                      << " protocol ip parent 1:0 prio 1 u32 match ip dst "
                      << target_ip_ << " flowid 1:1 2>/dev/null";
            std::cout << "[DEBUG] Executing: " << filter_cmd.str() << std::endl;
            system(filter_cmd.str().c_str());

        } else {
            // Apply to all traffic (simpler approach)
            std::stringstream cmd;
            cmd << "tc qdisc add dev " << default_iface_ << " root netem";

            if (lag_ms > 0) {
                cmd << " delay " << lag_ms << "ms";
            }

            if (drop_percent > 0 && drop_percent < 100) {
                cmd << " loss " << std::fixed << std::setprecision(2) << drop_percent << "%";
            }

            cmd << " 2>/dev/null";
            std::cout << "[DEBUG] Executing: " << cmd.str() << std::endl;
            system(cmd.str().c_str());
        }

        is_active_ = true;
        current_lag_ms_ = lag_ms;
        current_drop_percent_ = drop_percent;

        std::cout << "[SUCCESS] Network control applied!" << std::endl;
        if (!target_ip_.empty()) {
            std::cout << "  → Target IP: " << target_ip_ << std::endl;
        } else {
            std::cout << "  → Target: All traffic" << std::endl;
        }
        if (lag_ms > 0) {
            std::cout << "  → Ping increased by ~" << lag_ms << "ms (RTT: ~" << (lag_ms * 2) << "ms)" << std::endl;
        }
        if (drop_percent > 0) {
            std::cout << "  → Packet loss: " << drop_percent << "%" << std::endl;
        }

        return true;
    }

    bool disableLinux() {
        if (!default_iface_.empty()) {
            std::string cmd = "tc qdisc del dev " + default_iface_ + " root 2>/dev/null &";
            system(cmd.c_str());
        }

        system("iptables -w -D OUTPUT -j DROP 2>/dev/null & iptables -w -D INPUT -j DROP 2>/dev/null &");

        if (!target_ip_.empty()) {
            std::string cmd = "iptables -w -D OUTPUT -d " + target_ip_ + " -j DROP 2>/dev/null &";
            system(cmd.c_str());
        }

        is_active_ = false;
        current_lag_ms_ = 0;
        current_drop_percent_ = 0.0;
        return true;
    }
#endif
};

} // namespace netctrl
#endif

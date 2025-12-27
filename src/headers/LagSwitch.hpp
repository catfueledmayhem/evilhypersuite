#pragma once
#include <cstdlib>
#include <sstream>
#include "Globals.hpp"
#include "inpctrl.hpp"
#include "Helper.hpp"
#include "netctrl.hpp"
#include "logzz.hpp"

inline namespace LagSwitchNamespace {
    inline bool TrafficBlocked = false;
    inline bool PreventDisconnection = true;
    inline int LagTimeMilliseconds = 1;
    inline float PacketLossPercentage = 99.5f;
    inline bool customValuesAllowed = false;

    inline bool BlockTraffic() {
        if (TrafficBlocked) {
            log("Traffic already blocked");
            return true;
        }

        bool success = false;

        // Set the server IP target
        if (logzz::server_uses_udmux) {
            ctrl.setTargetIP(logzz::server_udmux_address);
        } else {
            ctrl.setTargetIP(logzz::server_rcc_address);
        }

        // Configure prevent disconnect mode
        ctrl.setPreventDisconnect(PreventDisconnection);

        log("Blocking outbound traffic for " + roblox_process_name);
        int lag_ms = customValuesAllowed ? LagTimeMilliseconds : 1;
        float drop_pct = customValuesAllowed ? PacketLossPercentage : 99.5f;

        if (PreventDisconnection) {
            // Apply lag + packet loss to prevent disconnection (uses WinDivert)
            log("[NetCtrl] Preventing disconnection: lag=" + std::to_string(lag_ms) +
                "ms, drop=" + std::to_string(drop_pct) + "%");
            success = ctrl.lag(lag_ms, static_cast<double>(drop_pct));
        } else {
            // Full block with firewall rules (will disconnect)
            log("[NetCtrl] Blocking all traffic (firewall mode - will disconnect)");
            success = ctrl.block();
        }

        if (success) {
            log("[NetCtrl] Successfully applied network control");
            TrafficBlocked = true;
        } else {
            log("[NetCtrl] Failed to apply network control");
        }

        return success;
    }

    inline bool UnblockTraffic() {
        if (!TrafficBlocked) {
            log("Traffic already unblocked");
            return true;
        }

        log("Unblocking outbound traffic for " + roblox_process_name);
        log("[NetCtrl] Disabling all network controls...");

        bool success = ctrl.disable();

        if (success) {
            TrafficBlocked = false;
            log("[NetCtrl] Successfully unblocked traffic");
        } else {
            log("[NetCtrl] Failed to disable network controls");
        }

        return success;
    }
}

inline void LagSwitch() {
    bool key_pressed = input.isKeyPressed(Binds["Lag-switch"]);

    if (!key_pressed && events[4]) {
        if (LagSwitchNamespace::TrafficBlocked) {
            LagSwitchNamespace::UnblockTraffic();
        } else {
            LagSwitchNamespace::BlockTraffic();
        }
    }

    events[4] = key_pressed;
}

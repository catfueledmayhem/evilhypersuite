#pragma once
#include "Globals.hpp"
#include "Macros.hpp"
#include "LagSwitch.hpp"
#include "Speedglitch.hpp"
#include "HHJ.hpp"
#include "GearDesync.hpp"
#include "WallhopAndWallwalk.hpp"
#include "FPSDropper.hpp"
#include "hsscript.hpp"
#include "hsscriptman.hpp"
#include "ImportedScriptsUI.hpp"

#include <thread>
#include <atomic>
#include <chrono>

namespace MacroSystem {

// Thread control
static std::atomic<bool> g_running{false};
static std::atomic<bool> g_initialized{false};
static std::thread g_macro_thread;

// Performance monitoring
static std::atomic<uint64_t> g_loop_count{0};

/// Initialize all macro systems
inline void initMacros() {
    try {
        initSpeedglitch();
        initHHJ();
        initGearDesync();
        initWallhop();
        initWallwalk();
        FpsDrop::Init();
        initScriptSystem();

        g_initialized.store(true);
        printf("[MacroSystem] Initialization complete\n");
    } catch (const std::exception& e) {
        fprintf(stderr, "[MacroSystem] Initialization error: %s\n", e.what());
        g_initialized.store(false);
    }
}

/// Main macro loop that runs on separate thread
inline void macroLoopThread() {
    printf("[MacroSystem] Macro loop thread started\n");

    while (g_running.load()) {
        auto frame_start = std::chrono::high_resolution_clock::now();

        if (g_initialized.load()) {
            try {
                // Execute all macros - same as before, just on a thread
                if (is_elevated) {}
                if (enabled[0]) freezeMacro();
                if (enabled[1]) laughClip();
                if (enabled[2]) extendedDanceClip();
                if (enabled[4]) LagSwitch();
                if (enabled[5]) BuckeyClip();
                if (enabled[6]) speedglitchMacro();
                if (enabled[7]) SpamKeyMacro();
                if (enabled[10]) DisableHeadCollision();
                if (enabled[11]) NHCRoofClip();
                if (enabled[12]) helicopterHighJump();
                //if (enabled[13]) gearDesyncMacro();
                if (enabled[14]) FullGearDesync();
                if (enabled[15]) FloorBounceHighJump();
                if (enabled[16]) wallhopMacro();
                if (enabled[17]) wallwalkMacro();
                if (enabled[18]) FpsDrop::Macro();

                HandleImportedScriptKeybindCapture(input);
                updateImportedScripts(input);

                g_loop_count.fetch_add(1);

            } catch (const std::exception& e) {
                fprintf(stderr, "[MacroSystem] Loop error: %s\n", e.what());
            }
        }

        // Small sleep to prevent CPU spinning (about 240Hz update rate)
        auto frame_end = std::chrono::high_resolution_clock::now();
        auto elapsed = std::chrono::duration_cast<std::chrono::microseconds>(frame_end - frame_start);

        constexpr auto TARGET_FRAME_TIME = std::chrono::microseconds(4167); // ~240Hz
        if (elapsed < TARGET_FRAME_TIME) {
            std::this_thread::sleep_for(TARGET_FRAME_TIME - elapsed);
        }
    }

    printf("[MacroSystem] Macro loop thread stopped\n");
}

// start the macro system
inline void StartMacroSystem() {
    if (g_running.load()) {
        fprintf(stderr, "[MacroSystem] Already running\n");
        return;
    }

    printf("[MacroSystem] Starting macro system...\n");

    initMacros();

    g_running.store(true);
    g_macro_thread = std::thread(macroLoopThread);

    printf("[MacroSystem] Macro system started\n");
}

// Stop the macro system
inline void StopMacroSystem() {
    if (!g_running.load()) {
        return;
    }

    printf("[MacroSystem] Stopping macro system...\n");

    g_running.store(false);

    if (g_macro_thread.joinable()) {
        g_macro_thread.join();
    }

    printf("[MacroSystem] Macro system stopped\n");
}

//-- legacy function for compatibility
inline void UpdateMacros() {
    if (!g_running.load()) {
        StartMacroSystem();
    }
}

/// Cleanup all macro systems (call at program exit)
inline void CleanupMacros() {
    printf("[MacroSystem] Cleaning up...\n");

    StopMacroSystem();

    try {
        cleanupWallhop();
        cleanupWallwalk();
        cleanupAllScripts();

        g_initialized.store(false);

        printf("[MacroSystem] Cleanup complete\n");
    } catch (const std::exception& e) {
        fprintf(stderr, "[MacroSystem] Cleanup error: %s\n", e.what());
    }
}

// Get performance statistics
inline void GetPerformanceStats(uint64_t& loops) {
    loops = g_loop_count.load();
}

// Reset performance counters
inline void ResetPerformanceStats() {
    g_loop_count.store(0);
}

// Check if macro system is healthy
inline bool IsSystemHealthy() {
    return g_running.load() && g_initialized.load();
}

} // namespace MacroSystem

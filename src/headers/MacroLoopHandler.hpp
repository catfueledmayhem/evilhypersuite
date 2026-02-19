#pragma once
#include "Globals.hpp"
#include "Macros.hpp"
#include "MacroThread.hpp"
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
static std::thread       g_macro_thread;

// Performance monitoring
static std::atomic<uint64_t> g_loop_count{0};

// ----------------------------------------------------------------
// initMacros - sets up the macros for first use ig
// ----------------------------------------------------------------
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

// ----------------------------------------------------------------
// macroLoopThread
//
// This loop only does key-detection and spawning.
// All macro *bodies* run on their own threads via MacroThread.
// at ~240 Hz and fires off threads as needed.
// ----------------------------------------------------------------

inline void macroLoopThread() {
    printf("[MacroSystem] Macro loop thread started\n");

    while (g_running.load()) {
        auto frame_start = std::chrono::high_resolution_clock::now();

        if (g_initialized.load()) {
            try {
                if (enabled[0])  freezeMacro();
                if (enabled[1])  laughClip();
                if (enabled[2])  extendedDanceClip();
                if (enabled[4])  LagSwitch();
                if (enabled[5])  BuckeyClip();
                if (enabled[6])  speedglitchMacro();
                if (enabled[7])  SpamKeyMacro();
                if (enabled[10]) DisableHeadCollision();
                if (enabled[11]) NHCRoofClip();
                if (enabled[12]) helicopterHighJump();
                //if (enabled[13]) gearDesyncMacro();
                if (enabled[14]) FullGearDesync();
                if (enabled[15]) FloorBounceHighJump();
                if (enabled[16]) wallhopMacro();
                if (enabled[17]) wallwalkMacro();
                if (enabled[18]) FpsDrop::Macro();

                // -- Imported / custom scripts --
                // Keybind capture is synchronous, it just reads input state and
                // writes to ImportedMacros[n].keybind, which is fine on this thread.
                HandleImportedScriptKeybindCapture(input);

                // Per-script threaded execution.
                // We replicate the keybind edge-detection from updateImportedScripts()
                // here so we can spawn each script independently without touching the
                // shared `static keyWasPressed` vector inside that function (which
                // would race if called from multiple threads simultaneously).
                for (size_t i = 0; i < ImportedMacros.size(); i++) {
                    ImportedScript& macro = ImportedMacros[i];

                    // Always compute isPressed so SpawnMacro receives the real
                    // key state every frame — this is what lets it track the
                    // rising/falling edge and re-arm after the key is released.
                    bool isPressed = (macro.enabled && macro.scriptIndex >= 0 && !macro.keybind.empty())
                                     ? isKeybindPressed(input, macro.keybind)
                                     : false;

                    // SpawnMacro handles edge-detection internally.
                    // It must be called every frame (even when isPressed=false)
                    // so it can update its internal wasHeld state on key release.
                    MacroThread::SpawnMacro("script_" + macro.name, isPressed, [i]() {
                        if (i >= ImportedMacros.size()) return;
                        const ImportedScript& m = ImportedMacros[i];
                        if (m.scriptIndex < 0 ||
                            static_cast<size_t>(m.scriptIndex) >= HSScriptClasses.size()) return;

                        HSScript* script = HSScriptClasses[m.scriptIndex].get();
                        if (script && script->functionExists("onExecute")) {
                            script->executeFunction("onExecute");
                        }
                    });
                }

                g_loop_count.fetch_add(1);

            } catch (const std::exception& e) {
                fprintf(stderr, "[MacroSystem] Loop error: %s\n", e.what());
            }
        }

        // ~240 Hz poll rate
        auto frame_end = std::chrono::high_resolution_clock::now();
        auto elapsed   = std::chrono::duration_cast<std::chrono::microseconds>(frame_end - frame_start);

        constexpr auto TARGET_FRAME_TIME = std::chrono::microseconds(4167);
        if (elapsed < TARGET_FRAME_TIME) {
            std::this_thread::sleep_for(TARGET_FRAME_TIME - elapsed);
        }
    }

    printf("[MacroSystem] Macro loop thread stopped\n");
}

// StartMacroSystem
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

// StopMacroSystem
inline void StopMacroSystem() {
    if (!g_running.load()) return;

    printf("[MacroSystem] Stopping macro system...\n");

    g_running.store(false);

    if (g_macro_thread.joinable()) {
        g_macro_thread.join();
    }

    printf("[MacroSystem] Macro system stopped\n");
}

// UpdateMacros
inline void UpdateMacros() {
    if (!g_running.load()) {
        StartMacroSystem();
    }
}

// CleanupMacros
inline void CleanupMacros() {
    printf("[MacroSystem] Cleaning up...\n");

    // Stop the poll loop first
    StopMacroSystem();

    // Wait for every in-flight macro thread to finish
    MacroThread::JoinAll();

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

// cool functions
inline void GetPerformanceStats(uint64_t& loops) {
    loops = g_loop_count.load();
}

inline void ResetPerformanceStats() {
    g_loop_count.store(0);
}

inline bool IsSystemHealthy() {
    return g_running.load() && g_initialized.load();
}

} // namespace MacroSystem

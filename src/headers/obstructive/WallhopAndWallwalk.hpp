#pragma once
#include "Globals.hpp"
#include "Helper.hpp"
#include <thread>
#include <atomic>
#include <chrono>
#include <cmath>

// ==================== WALLHOP ====================

// Wallhop Configuration
inline int wallhop_pixels = 300;           // Pixel amount for flick
inline int wallhop_degrees = 150;          // Degrees to rotate (estimated)
inline int wallhop_delay = 17;             // Length of wallhop in ms
inline int wallhop_bonus_delay = 0;        // Extra delay before jumping
inline bool wallhop_switch_side = false;   // Switch to left-flick
inline bool wallhop_toggle_jump = true;    // Jump during wallhop
inline bool wallhop_toggle_flick = true;   // Flick back after jump

inline std::atomic<bool> wallhop_active(false);
inline std::atomic<bool> wallhop_thread_should_exit(false);
inline std::thread wallhop_thread;

// Calculate wallhop pixels from degrees
inline void calculateWallhopPixels() {
    float sens = roblox_sensitivity;
    if (sens == 0.0f) sens = 0.5f; // Prevent division by zero

    float base_value = cam_fix_active ? 1000.0f : 720.0f;
    wallhop_pixels = static_cast<int>(std::round(
        (wallhop_degrees * base_value) / (360.0f * sens)
    ));

    log("Wallhop pixels calculated: " + std::to_string(wallhop_pixels) +
        " (for " + std::to_string(wallhop_degrees) + " degrees)");
}

// Calculate wallhop degrees from pixels
inline void calculateWallhopDegrees() {
    float sens = roblox_sensitivity;
    if (sens == 0.0f) sens = 0.5f;

    float base_value = cam_fix_active ? 1000.0f : 720.0f;
    wallhop_degrees = static_cast<int>(std::round(
        (360.0f * wallhop_pixels * sens) / base_value
    ));

    log("Wallhop degrees calculated: " + std::to_string(wallhop_degrees));
}

// Wallhop thread
inline void wallhopLoop() {
    log("Wallhop thread started");

    while (!wallhop_thread_should_exit.load(std::memory_order_relaxed)) {
        // Wait until wallhop is activated
        while (!wallhop_active.load(std::memory_order_relaxed) &&
               !wallhop_thread_should_exit.load(std::memory_order_relaxed)) {
            std::this_thread::sleep_for(std::chrono::milliseconds(10));
        }

        if (wallhop_thread_should_exit.load(std::memory_order_relaxed)) {
            break;
        }

        try {
            // Calculate flick amounts based on side
            int flick_x = wallhop_switch_side ? -wallhop_pixels : wallhop_pixels;
            int flick_y = wallhop_switch_side ? wallhop_pixels : -wallhop_pixels;

            // Initial flick
            input.moveMouse(flick_x, 0);

            // Handle bonus delay and jump
            if (wallhop_toggle_flick) {
                if (wallhop_bonus_delay > 0 && wallhop_bonus_delay < wallhop_delay) {
                    std::this_thread::sleep_for(std::chrono::milliseconds(wallhop_bonus_delay));

                    if (wallhop_toggle_jump) {
                        input.holdKey(CrossInput::Key::Space);
                    }

                    std::this_thread::sleep_for(std::chrono::milliseconds(wallhop_delay - wallhop_bonus_delay));
                } else {
                    if (wallhop_toggle_jump) {
                        input.holdKey(CrossInput::Key::Space);
                    }
                    std::this_thread::sleep_for(std::chrono::milliseconds(wallhop_delay));
                }

                // Flick back
                input.moveMouse(flick_y, 0);
            } else {
                // Just jump without flick-back
                if (wallhop_toggle_jump) {
                    input.holdKey(CrossInput::Key::Space);
                }
            }

            // Release jump
            if (wallhop_toggle_jump) {
                int remaining_delay = 100 - wallhop_delay;
                if (remaining_delay > 0) {
                    std::this_thread::sleep_for(std::chrono::milliseconds(remaining_delay));
                }
                input.releaseKey(CrossInput::Key::Space);
            }

        } catch (...) {
            log("ERROR: Exception in wallhop loop");
        }

        // Reset after each wallhop
        wallhop_active.store(false, std::memory_order_relaxed);
    }

    log("Wallhop thread exiting");
}

// Initialize wallhop
inline void initWallhop() {
    calculateWallhopPixels();
    wallhop_thread_should_exit.store(false, std::memory_order_relaxed);
    wallhop_thread = std::thread(wallhopLoop);
    log("Wallhop system initialized");
}

// Cleanup wallhop
inline void cleanupWallhop() {
    log("Cleaning up Wallhop");
    wallhop_active.store(false, std::memory_order_relaxed);
    wallhop_thread_should_exit.store(true, std::memory_order_relaxed);

    if (wallhop_thread.joinable()) {
        wallhop_thread.join();
    }
    log("Wallhop cleanup complete");
}

// Wallhop macro
inline void wallhopMacro() {
    bool key_pressed = input.isKeyPressed(Binds["Wallhop"]);

    // Trigger on press (not hold)
    if (key_pressed && !events[12]) {
        events[12] = true;
        wallhop_active.store(true, std::memory_order_relaxed);
        log("Wallhop triggered");
    }

    if (!key_pressed && events[12]) {
        events[12] = false;
    }
}

// Update functions
inline void updateWallhopPixels(int new_pixels) {
    wallhop_pixels = new_pixels;
    calculateWallhopDegrees();
}

inline void updateWallhopDegrees(int new_degrees) {
    wallhop_degrees = new_degrees;
    calculateWallhopPixels();
}

inline void updateWallhopDelay(int new_delay) {
    wallhop_delay = new_delay;
    log("Wallhop delay updated to: " + std::to_string(new_delay) + "ms");
}

// ==================== WALLWALK ====================

// Wallwalk Configuration
inline int wallwalk_pixels_x = 94;         // Horizontal flick pixels
inline int wallwalk_pixels_y = -94;        // Vertical flick pixels (negative)
inline int wallwalk_delay = 72720;         // Delay between flicks in microseconds
inline bool wallwalk_switch_side = false;  // Switch to left-side
inline bool wallwalk_is_toggle = false;    // Toggle or hold mode

inline std::atomic<bool> wallwalk_active(false);
inline std::atomic<bool> wallwalk_thread_should_exit(false);
inline std::thread wallwalk_thread;

// Calculate wallwalk pixels from sensitivity
inline void calculateWallwalkPixels() {
    float sens = roblox_sensitivity;
    if (sens == 0.0f) sens = 0.5f;

    float base_value = cam_fix_active ? 500.0f : 360.0f;
    int calculated = static_cast<int>(std::round((base_value / sens) * 0.13f));

    wallwalk_pixels_x = calculated;
    wallwalk_pixels_y = -calculated;

    log("Wallwalk pixels calculated: " + std::to_string(wallwalk_pixels_x));
}

// Wallwalk loop
inline void wallwalkLoop() {
    log("Wallwalk thread started");

    int frame_delay = 16; // Will be updated based on FPS

    while (!wallwalk_thread_should_exit.load(std::memory_order_relaxed)) {
        // Wait until wallwalk is activated
        while (!wallwalk_active.load(std::memory_order_relaxed) &&
               !wallwalk_thread_should_exit.load(std::memory_order_relaxed)) {
            std::this_thread::sleep_for(std::chrono::milliseconds(10));
            // Update frame delay based on FPS
            frame_delay = static_cast<int>(std::round((1000.0f / roblox_fps) * 1.1f));
        }

        if (wallwalk_thread_should_exit.load(std::memory_order_relaxed)) {
            break;
        }

        try {
            // Determine flick direction
            int flick_x = wallwalk_switch_side ? -wallwalk_pixels_x : wallwalk_pixels_x;
            int flick_y = wallwalk_switch_side ? -wallwalk_pixels_y : wallwalk_pixels_y;

            // Perform wallwalk sequence
            input.moveMouse(flick_x, 0);
            std::this_thread::sleep_for(std::chrono::milliseconds(frame_delay));
            input.moveMouse(flick_y, 0);
            std::this_thread::sleep_for(std::chrono::microseconds(wallwalk_delay));

        } catch (...) {
            log("ERROR: Exception in wallwalk loop");
            wallwalk_active.store(false, std::memory_order_relaxed);
            break;
        }
    }

    log("Wallwalk thread exiting");
}

// Initialize wallwalk
inline void initWallwalk() {
    calculateWallwalkPixels();
    wallwalk_thread_should_exit.store(false, std::memory_order_relaxed);
    wallwalk_thread = std::thread(wallwalkLoop);
    log("Wallwalk system initialized");
}

// Cleanup wallwalk
inline void cleanupWallwalk() {
    log("Cleaning up Wallwalk");
    wallwalk_active.store(false, std::memory_order_relaxed);
    wallwalk_thread_should_exit.store(true, std::memory_order_relaxed);

    if (wallwalk_thread.joinable()) {
        wallwalk_thread.join();
    }
    log("Wallwalk cleanup complete");
}

// Wallwalk macro
inline void wallwalkMacro() {
    bool key_pressed = input.isKeyPressed(Binds["Wallwalk"]);

    if (wallwalk_is_toggle) {
        // Toggle mode
        if (key_pressed && !events[13]) {
            events[13] = true;
            wallwalk_active = !wallwalk_active.load(std::memory_order_relaxed);

            if (wallwalk_active) {
                log("Wallwalk activated (toggle)");
            } else {
                log("Wallwalk deactivated (toggle)");
            }
        }

        if (!key_pressed && events[13]) {
            events[13] = false;
        }
    } else {
        // Hold mode
        if (key_pressed) {
            if (!wallwalk_active.load(std::memory_order_relaxed)) {
                wallwalk_active.store(true, std::memory_order_relaxed);
                log("Wallwalk activated (hold)");
            }
        } else {
            if (wallwalk_active.load(std::memory_order_relaxed)) {
                wallwalk_active.store(false, std::memory_order_relaxed);
                log("Wallwalk deactivated (hold)");
            }
        }
    }
}

// Update functions
inline void updateWallwalkPixels(int new_pixels) {
    wallwalk_pixels_x = new_pixels;
    wallwalk_pixels_y = -new_pixels;
    log("Wallwalk pixels updated to: " + std::to_string(new_pixels));
}

inline void updateWallwalkDelay(int new_delay) {
    wallwalk_delay = new_delay;
    log("Wallwalk delay updated to: " + std::to_string(new_delay) + "µs");
}

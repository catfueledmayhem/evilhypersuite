#pragma once
#include "Globals.hpp"
#include "Helper.hpp"
#include <thread>
#include <atomic>
#include <chrono>

// HHJ-specific speedglitch loop (runs in separate thread)
inline void hhjSpeedglitchLoop()
{
    int sleep1 = 16, sleep2 = 16;
    int last_fps = 0;
    const float EPSILON = 0.008f;

    while (true) {
        // Wait until HHJ speedglitch is activated
        while (!hhj_speedglitch_active.load(std::memory_order_relaxed)) {
            std::this_thread::sleep_for(std::chrono::milliseconds(1));
        }

        // Recalculate delays if FPS changed
        if (last_fps != roblox_fps) {
            float delay_float = 1000.0f / static_cast<float>(roblox_fps);
            int delay_floor = static_cast<int>(delay_float);
            int delay_ceil = delay_floor + 1;
            float fractional = delay_float - delay_floor;

            if (fractional < 0.33f - EPSILON)
            {
                sleep1 = sleep2 = delay_floor;
            }
            else if (fractional > 0.66f + EPSILON)
            {
                sleep1 = sleep2 = delay_ceil;
            }
            else
            {
                sleep1 = delay_floor;
                sleep2 = delay_ceil;
            }

            last_fps = roblox_fps;
        }

        // Perform the speedglitch rotation
        input.moveMouse(speed_pixels_x, 0);
        std::this_thread::sleep_for(std::chrono::milliseconds(sleep1));
        input.moveMouse(speed_pixels_y, 0);
        std::this_thread::sleep_for(std::chrono::milliseconds(sleep2));
    }
}

// Initialize HHJ system
inline void initHHJ()
{
    hhj_speedglitch_thread = std::thread(hhjSpeedglitchLoop);
    hhj_speedglitch_thread.detach();
    log("HHJ system initialized");
}

// Main HHJ Macro Function
inline void helicopterHighJump()
{
    static bool last_key_state = false;
    bool key_pressed = input.isKeyPressed(Binds["HHJ"]);

    // Trigger on key press (not hold)
    if (key_pressed && !last_key_state)
    {
        log("HHJ triggered");

            float base_value = cam_fix_active ? 500.0f : 360.0f;
            float multiplier = (359.0f / 360.0f) * (359.0f / 360.0f); // Slight adjustment for accuracy

            speed_pixels_x = static_cast<int>(std::round((base_value / roblox_sensitivity) * multiplier));
            speed_pixels_y = -speed_pixels_x; // Opposite direction
            input.moveMouse(speed_pixels_x, 0);  // Rotate 180° one ways 
        log("HHJ: Spinning stopped");

        log("HHJ completed");
    }

    last_key_state = key_pressed;
}

// Update HHJ settings functions
inline void updateHHJLength(int new_length)
{
    hhj_length = new_length;
    log("HHJ length updated to: " + std::to_string(new_length) + "ms");
}

inline void updateHHJDelays(int delay1, int delay2, int delay3)
{
    hhj_delay1 = delay1;
    hhj_delay2 = delay2;
    hhj_delay3 = delay3;
    log("HHJ delays updated: " + std::to_string(delay1) + ", " +
        std::to_string(delay2) + ", " + std::to_string(delay3));
}

inline void updateHHJFreezeDelay(int delay)
{
    hhj_freeze_delay = delay;
    log("HHJ freeze delay override: " + std::to_string(delay) + "ms");
}

inline void setHHJAutoTiming(bool enabled)
{
    hhj_auto_timing = enabled;
    log("HHJ auto-timing: " + std::string(enabled ? "enabled" : "disabled"));
}

inline void setHHJFastMode(bool enabled)
{
    hhj_fast_mode = enabled;
    log("HHJ fast mode: " + std::string(enabled ? "enabled" : "disabled"));
}

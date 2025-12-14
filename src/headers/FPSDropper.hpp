#pragma once
#include "Globals.hpp"
#include "procctrl.hpp"

namespace FpsDrop {
    inline std::atomic<bool> active(false);
    inline std::thread thread;
    inline unsigned int target_fps = 30;
    inline int pid = -1;
    inline static char fps_input[16];
    inline void Loop() {
        while (true) {
            while (!active.load(std::memory_order_relaxed)) {
                std::this_thread::sleep_for(std::chrono::milliseconds(1));
            }

            if (pid == -1) {
                std::this_thread::sleep_for(std::chrono::milliseconds(1));
                continue;
            }

            // Calculate timings
            int ft = 1000 / target_fps;
            int current_user_ft = 1000 / 60;
            int freeze_time = ft - current_user_ft;

            if (freeze_time > 0) {
                procctrl::set_process_suspended(pid, true);
                std::this_thread::sleep_for(std::chrono::milliseconds(freeze_time));
                procctrl::set_process_suspended(pid, false);
                std::this_thread::sleep_for(std::chrono::milliseconds(current_user_ft));
            }

            std::this_thread::sleep_for(std::chrono::milliseconds(1));
        }
    }

    inline void Macro() {
        static bool last_key_state = false;
        bool key_pressed = input.isKeyPressed(Binds["FPS-Drop"]);

        if (key_pressed && !last_key_state) {
            if (!active.load(std::memory_order_relaxed)) {
                pid = procctrl::find_process_by_name(roblox_process_name);
                if (pid == -1) {
                    log("Process not found!");
                    last_key_state = key_pressed;
                    return;
                }
            }

            active = !active.load(std::memory_order_relaxed);

            if (active) {
                log("Fps dropper activated");
            } else {
                if (pid != -1) {
                    procctrl::set_process_suspended(pid, false);
                }
                log("Fps dropper deactivated");
            }
        }

        last_key_state = key_pressed;
    }

    inline void Init() {
        thread = std::thread(Loop);
        thread.detach();
        log("fps drop system initialized");
    }
}

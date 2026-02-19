#pragma once
#include "Globals.hpp"
#include "Helper.hpp"
#include "MacroThread.hpp"

#ifdef _WIN32
#include "clipctrl.hpp"
#endif

// ================================================================
//  chat_handler  (unchanged — these are called from inside threads)
// ================================================================
inline namespace chat_handler {
    inline void paste_text() {
        input.holdKey(CrossInput::Key::LCtrl);
        input.holdKey(CrossInput::Key::V);
        std::this_thread::sleep_for(std::chrono::milliseconds(40));
        input.releaseKey(CrossInput::Key::LCtrl);
        input.releaseKey(CrossInput::Key::V);
    }

    inline void type_laugh() {
#ifdef _WIN32
        clipboard_set_text("/e laugh");
        paste_text();
#else
        if (kb_layout == 1) {
            typeSlashAzerty();
            input.typeText("e lqugh");
        } else {
            input.typeText("/e laugh");
        }
#endif
    }

    inline void type_dance2() {
#ifdef _WIN32
        clipboard_set_text("/e dance2");
        paste_text();
#else
        if (kb_layout == 1) {
            typeSlashAzerty();
            input.typeText("e dqnce");
            input.holdKey(CrossInput::Key::LShift);
            input.pressKey(CrossInput::Key::Num2);
            input.releaseKey(CrossInput::Key::LShift);
            std::this_thread::sleep_for(std::chrono::milliseconds(30));
            input.pressKey(CrossInput::Key::Enter);
        } else {
            input.typeText("/e dance2");
            input.pressKey(CrossInput::Key::Enter);
        }
#endif
    }

    inline void type_cheer() {
#ifdef _WIN32
        clipboard_set_text("/e cheer");
        paste_text();
#else
        if (kb_layout == 1) {
            typeSlashAzerty();
            input.typeText("e cheer", 20);
        } else {
            input.typeText("/e cheer", 20);
        }
#endif
    }
}

// ================================================================
//  freezeMacro
//  Not threaded — this is a toggle that must stay on the main loop.
// ================================================================
inline void freezeMacro() {
    bool key_pressed = input.isKeyPressed(Binds["Freeze"]);

    if (key_pressed && !events[0]) {
        log("Freeze triggered for " + roblox_process_name);
        procctrl::suspend_processes_by_name(roblox_process_name);
    }

    if (!key_pressed && events[0]) {
        log("Unfreeze triggered for " + roblox_process_name);
        procctrl::resume_processes_by_name(roblox_process_name);
    }

    events[0] = key_pressed;
}

// ================================================================
//  laughClip
// ================================================================
inline void laughClip() {
    bool key_pressed = input.isKeyPressed(Binds["Laugh"]);

    MacroThread::SpawnMacro("laughClip", key_pressed, []() {
        log("Laugh clip triggered");

        input.pressKey(ChatKey);
        std::this_thread::sleep_for(std::chrono::milliseconds(100));
        chat_handler::type_laugh();
        input.pressKey(CrossInput::Key::Enter);

        std::this_thread::sleep_for(std::chrono::milliseconds(248));
        input.holdKey(CrossInput::Key::S);
        procctrl::suspend_processes_by_name(roblox_process_name);

        std::this_thread::sleep_for(std::chrono::milliseconds(500));
        procctrl::resume_processes_by_name(roblox_process_name);

        input.holdKey(CrossInput::Key::Space);
        input.holdKey(CrossInput::Key::LShift);
        std::this_thread::sleep_for(std::chrono::milliseconds(35));
        input.releaseKey(CrossInput::Key::Space);
        input.releaseKey(CrossInput::Key::LShift);

        std::this_thread::sleep_for(std::chrono::milliseconds(40));
        input.releaseKey(CrossInput::Key::S);

        log("Laugh clip finished");
    });
}

// ================================================================
//  extendedDanceClip
// ================================================================
inline void extendedDanceClip() {
    bool key_pressed = input.isKeyPressed(Binds["E-Dance"]);

    MacroThread::SpawnMacro("extendedDanceClip", key_pressed, []() {
        log("Extended Dance clip triggered");

        input.pressKey(ChatKey);
        std::this_thread::sleep_for(std::chrono::milliseconds(100));
        chat_handler::type_dance2();
        input.pressKey(CrossInput::Key::Enter);

        std::this_thread::sleep_for(std::chrono::milliseconds(815));

        input.holdKey(CrossInput::Key::D);
        procctrl::suspend_processes_by_name(roblox_process_name);

        std::this_thread::sleep_for(std::chrono::milliseconds(500));
        procctrl::resume_processes_by_name(roblox_process_name);

        input.pressKey(CrossInput::Key::LShift);
        std::this_thread::sleep_for(std::chrono::milliseconds(300));
        input.releaseKey(CrossInput::Key::D);

        log("Extended Dance clip finished");
    });
}

// ================================================================
//  BuckeyClip
// ================================================================
inline void BuckeyClip() {
    bool key_pressed = input.isKeyPressed(Binds["Buckey-clip"]);

    MacroThread::SpawnMacro("buckeyClip", key_pressed, []() {
        log("Buckey clip triggered");

        input.pressKey(ChatKey);
        std::this_thread::sleep_for(std::chrono::milliseconds(100));
        chat_handler::type_laugh();
        input.pressKey(CrossInput::Key::Enter);

        std::this_thread::sleep_for(std::chrono::milliseconds(200));

        input.holdKey(CrossInput::Key::Space);

        std::this_thread::sleep_for(std::chrono::milliseconds(31));
        input.holdKey(CrossInput::Key::S);

        std::this_thread::sleep_for(std::chrono::milliseconds(9));
        input.holdKey(CrossInput::Key::LShift);

        std::this_thread::sleep_for(std::chrono::milliseconds(74));
        input.releaseKey(CrossInput::Key::Space);

        std::this_thread::sleep_for(std::chrono::milliseconds(56));
        input.releaseKey(CrossInput::Key::LShift);

        std::this_thread::sleep_for(std::chrono::milliseconds(82));
        input.releaseKey(CrossInput::Key::S);

        log("Buckeyclip finished");
    });
}

// ================================================================
//  SpamKeyMacro
// ================================================================
inline void SpamKeyMacro() {
    bool key_pressed = input.isKeyPressed(Binds["Spam-Key"]);

    MacroThread::SpawnMacro("spamKey", key_pressed, []() {
        log("Spam key triggering");
        input.pressKey(SpamKey, 1);
        log("Spam key finished triggering");
    });
}

// ================================================================
//  DisableHeadCollision
// ================================================================
inline void DisableHeadCollision() {
    bool key_pressed = input.isKeyPressed(Binds["Disable-Head-Collision"]);

    MacroThread::SpawnMacro("disableHeadCollision", key_pressed, []() {
        log("Disable-Head-Collision triggered");

        input.pressKey(ChatKey);
        std::this_thread::sleep_for(std::chrono::milliseconds(248));
        chat_handler::type_laugh();
        input.pressKey(CrossInput::Key::Enter);

        std::this_thread::sleep_for(std::chrono::milliseconds(200));

        input.holdKey(CrossInput::Key::LShift);
        std::this_thread::sleep_for(std::chrono::milliseconds(16));
        input.releaseKey(CrossInput::Key::LShift);

        input.holdKey(CrossInput::Key::Space);
        std::this_thread::sleep_for(std::chrono::milliseconds(16));
        input.releaseKey(CrossInput::Key::Space);

        input.holdKey(CrossInput::Key::LShift);
        std::this_thread::sleep_for(std::chrono::milliseconds(16));
        input.releaseKey(CrossInput::Key::LShift);

        log("Disable-Head-Collision finished");
    });
}

// ================================================================
//  NHCRoofClip
// ================================================================
inline void NHCRoofClip() {
    bool key_pressed = input.isKeyPressed(Binds["NHC-Roof"]);

    MacroThread::SpawnMacro("nhcRoofClip", key_pressed, []() {
        log("NHC-Roof clip triggered");

        input.pressKey(ChatKey);
        std::this_thread::sleep_for(std::chrono::milliseconds(248));
        chat_handler::type_cheer();

        std::this_thread::sleep_for(std::chrono::milliseconds(100));
        input.pressKey(CrossInput::Key::Enter);

        std::this_thread::sleep_for(std::chrono::milliseconds(610));

        input.holdKey(CrossInput::Key::Space);
        std::this_thread::sleep_for(std::chrono::milliseconds(20));
        procctrl::suspend_processes_by_name(roblox_process_name);
        std::this_thread::sleep_for(std::chrono::milliseconds(300));
        procctrl::resume_processes_by_name(roblox_process_name);
        std::this_thread::sleep_for(std::chrono::milliseconds(20));
        input.releaseKey(CrossInput::Key::Space);

        log("NHC-Roof clip finished");
    });
}

// ================================================================
//  FullGearDesync
// ================================================================
inline void FullGearDesync() {
    bool key_pressed = input.isKeyPressed(Binds["Full-Gear-Desync"]);

    MacroThread::SpawnMacro("fullGearDesync", key_pressed, []() {
        log("Full Gear Desync triggered");

        input.pressKey(CrossInput::Key::Num2);
        std::this_thread::sleep_for(std::chrono::milliseconds(40));
        input.pressKey(CrossInput::Key::Backspace);
        std::this_thread::sleep_for(std::chrono::milliseconds(200));
        input.pressKey(CrossInput::Key::Num1);
        std::this_thread::sleep_for(std::chrono::milliseconds(40));

        input.holdKey(CrossInput::Key::W);
        std::this_thread::sleep_for(std::chrono::milliseconds(400));
        procctrl::suspend_processes_by_name(roblox_process_name);

        input.pressKey(CrossInput::Key::Num1, 30);
        std::this_thread::sleep_for(std::chrono::milliseconds(30));
        input.pressKey(CrossInput::Key::Num1, 30);
        std::this_thread::sleep_for(std::chrono::milliseconds(100));
        procctrl::resume_processes_by_name(roblox_process_name);

        input.releaseKey(CrossInput::Key::W);
        std::this_thread::sleep_for(std::chrono::milliseconds(40));
        input.pressKey(CrossInput::Key::Num1);
        std::this_thread::sleep_for(std::chrono::milliseconds(40));
        input.pressKey(CrossInput::Key::Backspace);

        log("Full Gear Desync finished");
    });
}

// ================================================================
//  FloorBounceHighJump
// ================================================================
inline void FloorBounceHighJump() {
    bool key_pressed = input.isKeyPressed(Binds["Floor-Bounce-High-Jump"]);

    MacroThread::SpawnMacro("floorBounceHighJump", key_pressed, []() {
        log("Floor bounce high jump triggered");

        input.holdKey(CrossInput::Key::Space);
        std::this_thread::sleep_for(std::chrono::milliseconds(521));  // Fall timing

        procctrl::suspend_processes_by_name(roblox_process_name);     // Clip through floor
        std::this_thread::sleep_for(std::chrono::milliseconds(72));   // Stay clipped

        procctrl::resume_processes_by_name(roblox_process_name);      // Register underground position
        std::this_thread::sleep_for(std::chrono::milliseconds(72));   // Let correction force build

        procctrl::suspend_processes_by_name(roblox_process_name);     // Freeze the ejection force
        std::this_thread::sleep_for(std::chrono::milliseconds(72));   // Hold the power

        procctrl::resume_processes_by_name(roblox_process_name);      // LAUNCH
        std::this_thread::sleep_for(std::chrono::milliseconds(100));

        input.releaseKey(CrossInput::Key::Space);

        log("Floor bounce high jump finished");
    });
}

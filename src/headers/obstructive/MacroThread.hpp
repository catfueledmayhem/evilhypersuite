#pragma once
#include <thread>
#include <atomic>
#include <functional>
#include <unordered_map>
#include <mutex>
#include <vector>
#include <string>
#include <algorithm>

namespace MacroThread {

// ----------------------------------------------------------------
// MacroInstance
// Represents a single running invocation of a macro.
// The thread detaches immediately so SpawnMacro never blocks,
// but we still track liveness via `running` so we can cap
// concurrent instances and clean up on shutdown.
// ----------------------------------------------------------------
struct MacroInstance {
    std::thread       thread;
    std::atomic<bool> running{false};

    MacroInstance() = default;

    // Non-copyable, non-movable after construction (atomic + thread)
    MacroInstance(const MacroInstance&)            = delete;
    MacroInstance& operator=(const MacroInstance&) = delete;
};

// Internal state or something idk
static std::unordered_map<std::string, std::vector<MacroInstance*>> g_instances;
static std::mutex g_mutex;

// stupid name but whatever you get what i mean
constexpr size_t MAX_INSTANCES_PER_MACRO = 4;

// ----------------------------------------------------------------
// Helpers
// ----------------------------------------------------------------

/// Remove and delete any instances that have already finished.
/// Must be called with g_mutex held.
inline void _pruneFinished(std::vector<MacroInstance*>& vec) {
    vec.erase(
        std::remove_if(vec.begin(), vec.end(),
            [](MacroInstance* inst) {
                if (!inst->running.load()) {
                    // Thread already detached, just free the object
                    delete inst;
                    return true;
                }
                return false;
            }),
        vec.end()
    );
}

inline void SpawnMacro(const std::string& name, bool key_held, std::function<void()> fn) {
    std::lock_guard<std::mutex> lock(g_mutex);

    // -- Rising-edge detection (one entry per named macro) --
    static std::unordered_map<std::string, bool> s_wasHeld;
    bool was_held = s_wasHeld[name]; // default-constructs false on first access
    s_wasHeld[name] = key_held;

    // Only fire on the transition from not-held -> held
    if (!key_held || was_held) return;

    auto& instances = g_instances[name];

    _pruneFinished(instances);

    if (instances.size() >= MAX_INSTANCES_PER_MACRO) {
        return;
    }

    // Heap-allocate the thread lambda captures this pointer and the
    // object must outlive the spawning scope
    MacroInstance* inst = new MacroInstance();
    inst->running.store(true);

    inst->thread = std::thread([inst, fn]() {
        try {
            fn();
        } catch (...) {
            // Swallow so the thread never terminates abnormally
        }
        inst->running.store(false);
        // deleted next time _pruneFinished runs for this macro
    });

    // Detach — lifetime is tracked via the `running` atomic
    inst->thread.detach();

    instances.push_back(inst);
}

// ----------------------------------------------------------------
// JoinAll
//
// Call during program shutdown (inside CleanupMacros).
// Waits for every still-running instance across all macros to finish,
// then frees all memory.
// ----------------------------------------------------------------
inline void JoinAll() {
    // We can't join detached threads, so we spin-wait on the atomic flags.
    // This is only called at shutdown so the brief busy-wait is acceptable.
    bool any_alive = true;
    while (any_alive) {
        any_alive = false;
        std::lock_guard<std::mutex> lock(g_mutex);
        for (auto& [name, vec] : g_instances) {
            for (auto* inst : vec) {
                if (inst->running.load()) {
                    any_alive = true;
                }
            }
        }
        if (any_alive) {
            std::this_thread::sleep_for(std::chrono::milliseconds(10));
        }
    }

    // All done — free everything
    std::lock_guard<std::mutex> lock(g_mutex);
    for (auto& [name, vec] : g_instances) {
        for (auto* inst : vec) {
            delete inst;
        }
    }
    g_instances.clear();
}

inline size_t ActiveCount(const std::string& name) {
    std::lock_guard<std::mutex> lock(g_mutex);
    auto it = g_instances.find(name);
    if (it == g_instances.end()) return 0;
    _pruneFinished(it->second);
    return it->second.size();
}

inline void WaitForMacro(const std::string& name) {
    bool any_alive = true;
    while (any_alive) {
        any_alive = false;
        {
            std::lock_guard<std::mutex> lock(g_mutex);
            auto it = g_instances.find(name);
            if (it != g_instances.end()) {
                for (auto* inst : it->second) {
                    if (inst->running.load()) {
                        any_alive = true;
                        break;
                    }
                }
            }
        }
        if (any_alive) {
            std::this_thread::sleep_for(std::chrono::milliseconds(10));
        }
    }
}

} // namespace MacroThread

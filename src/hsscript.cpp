#include "hsscript.hpp"
#include "hsscriptman.hpp"
#include "Helper.hpp"
#include "LagSwitch.hpp"
#include "logzz.hpp"
#include "procctrl.hpp"
#include "Globals.hpp"
#include <iostream>
#include <fstream>
#include <lua.h>
#include <thread>
#include <chrono>
#include <unordered_map>
#include <cstdint>

// Initialize static member
HSScript* HSScript::s_instance = nullptr;

HSScript::HSScript() : L(nullptr), m_initialized(false) {
}

HSScript::~HSScript() {
    cleanup();
}

bool HSScript::init() {
    if (m_initialized) {
        return true;
    }

    // Initialize CrossInput
    if (!m_input.init()) {
        std::cerr << "Failed to initialize input system!" << std::endl;
        return false;
    }

    // Create Lua state
    L = luaL_newstate();
    if (!L) {
        std::cerr << "Failed to create Lua state!" << std::endl;
        m_input.cleanup();
        return false;
    }

    // Load standard Lua libraries
    luaL_openlibs(L);

    // Set global instance for callbacks
    s_instance = this;

    // Register all input functions
    registerInputFunctions();

    m_initialized = true;
    std::cout << "HSScript initialized successfully" << std::endl;
    return true;
}

void HSScript::cleanup() {
    if (!m_initialized) {
        return;
    }

    if (L) {
        lua_close(L);
        L = nullptr;
    }

    m_input.cleanup();
    s_instance = nullptr;
    m_initialized = false;
}

bool HSScript::loadFile(const std::string& filename) {
    if (!m_initialized) {
        std::cerr << "HSScript not initialized!" << std::endl;
        return false;
    }

    // Check if file exists
    std::ifstream file(filename);
    if (!file.good()) {
        std::cerr << "File not found: " << filename << std::endl;
        return false;
    }
    file.close();

    // Load and execute the file
    if (luaL_dofile(L, filename.c_str()) != LUA_OK) {
        std::cerr << "Error loading file '" << filename << "': "
                  << lua_tostring(L, -1) << std::endl;
        lua_pop(L, 1);
        return false;
    }

    std::cout << "Loaded script: " << filename << std::endl;
    return true;
}

bool HSScript::executeString(const std::string& code) {
    if (!m_initialized) {
        std::cerr << "HSScript not initialized!" << std::endl;
        return false;
    }

    if (luaL_dostring(L, code.c_str()) != LUA_OK) {
        std::cerr << "Error executing code: " << lua_tostring(L, -1) << std::endl;
        lua_pop(L, 1);
        return false;
    }

    return true;
}

bool HSScript::executeFunction(const std::string& funcName) {
    if (!m_initialized) {
        std::cerr << "HSScript not initialized!" << std::endl;
        return false;
    }

    lua_getglobal(L, funcName.c_str());

    if (!lua_isfunction(L, -1)) {
        std::cerr << "Function '" << funcName << "' not found!" << std::endl;
        lua_pop(L, 1);
        return false;
    }

    if (lua_pcall(L, 0, 0, 0) != LUA_OK) {
        std::cerr << "Error calling function '" << funcName << "': "
                  << lua_tostring(L, -1) << std::endl;
        lua_pop(L, 1);
        return false;
    }

    return true;
}

bool HSScript::executeFunctionWithString(const std::string& funcName, const std::string& arg) {
    if (!m_initialized) {
        std::cerr << "HSScript not initialized!" << std::endl;
        return false;
    }

    lua_getglobal(L, funcName.c_str());

    if (!lua_isfunction(L, -1)) {
        std::cerr << "Function '" << funcName << "' not found!" << std::endl;
        lua_pop(L, 1);
        return false;
    }

    lua_pushstring(L, arg.c_str());

    if (lua_pcall(L, 1, 0, 0) != LUA_OK) {
        std::cerr << "Error calling function '" << funcName << "': "
                  << lua_tostring(L, -1) << std::endl;
        lua_pop(L, 1);
        return false;
    }

    return true;
}

bool HSScript::executeFunctionWithNumber(const std::string& funcName, double arg) {
    if (!m_initialized) {
        std::cerr << "HSScript not initialized!" << std::endl;
        return false;
    }

    lua_getglobal(L, funcName.c_str());

    if (!lua_isfunction(L, -1)) {
        std::cerr << "Function '" << funcName << "' not found!" << std::endl;
        lua_pop(L, 1);
        return false;
    }

    lua_pushnumber(L, arg);

    if (lua_pcall(L, 1, 0, 0) != LUA_OK) {
        std::cerr << "Error calling function '" << funcName << "': "
                  << lua_tostring(L, -1) << std::endl;
        lua_pop(L, 1);
        return false;
    }

    return true;
}

bool HSScript::functionExists(const std::string& funcName) {
    if (!m_initialized) {
        return false;
    }

    lua_getglobal(L, funcName.c_str());
    bool exists = lua_isfunction(L, -1);
    lua_pop(L, 1);
    return exists;
}

void HSScript::setGlobalNumber(const std::string& varName, double value) {
    if (!m_initialized) return;
    lua_pushnumber(L, value);
    lua_setglobal(L, varName.c_str());
}

void HSScript::setGlobalString(const std::string& varName, const std::string& value) {
    if (!m_initialized) return;
    lua_pushstring(L, value.c_str());
    lua_setglobal(L, varName.c_str());
}

double HSScript::getGlobalNumber(const std::string& varName) {
    if (!m_initialized) return 0.0;
    lua_getglobal(L, varName.c_str());
    double value = lua_tonumber(L, -1);
    lua_pop(L, 1);
    return value;
}

std::string HSScript::getGlobalString(const std::string& varName) {
    if (!m_initialized) return "";
    lua_getglobal(L, varName.c_str());
    std::string value = lua_tostring(L, -1);
    lua_pop(L, 1);
    return value;
}

void HSScript::registerFunction(const std::string& name, lua_CFunction func) {
    if (!m_initialized) return;
    lua_register(L, name.c_str(), func);
}

void HSScript::registerInputFunctions() {
    lua_register(L, "pressKey", lua_pressKey);
    lua_register(L, "holdKey", lua_holdKey);
    lua_register(L, "releaseKey", lua_releaseKey);
    lua_register(L, "isKeyPressed", lua_isKeyPressed);
    lua_register(L, "typeText", lua_typeText);
    lua_register(L, "moveMouse", lua_moveMouse);
    lua_register(L, "turnDegrees", lua_turnDegrees);
    lua_register(L, "sleep", lua_sleep);
    lua_register(L, "log", lua_log);
    lua_register(L, "waitForKey", lua_waitForKey);
    lua_register(L, "getKeyName", lua_getKeyName);
    lua_register(L, "clear", lua_clear);
    lua_register(L, "lagSwitchMan", lua_lagswitch);
    lua_register(L, "robloxFreeze", lua_roblox_freeze);
    lua_register(L, "triggerMacro", lua_trigger_macro);
}

// ==================== LUA CALLBACK FUNCTIONS ====================

int HSScript::lua_clear(lua_State* L) {
    if (!s_instance) {
        lua_pushstring(L, "HSScript instance not available");
        lua_error(L);
        return 0;
    }
    clearConsole(); // from helper.hpp twin
    return 0;
}

int HSScript::lua_roblox_freeze(lua_State* L) {
    if (!s_instance) {
        lua_pushstring(L, "HSScript instance not available");
        lua_error(L);
        return 0;
    }
    int argCount = lua_gettop(L);
    // check at least 1 arg
    if (argCount < 1) {
        return luaL_error(L, "Expected at least 1 argument");
    }

    // arg 1 should be boolean
    if (!lua_isboolean(L, 1)) {
        return luaL_error(L, "Bad argument #1 (expected boolean)");
    }
    bool enable = lua_toboolean(L, 1);

    if (enable) {
        procctrl::suspend_processes_by_name(roblox_process_name);
    } else {
        procctrl::resume_processes_by_name(roblox_process_name);
    }
    return 0;
}
int HSScript::lua_trigger_macro(lua_State* L) {
    if (!s_instance) {
        lua_pushstring(L, "HSScript instance not available");
        lua_error(L);
        return 0;
    }
    int argCount = lua_gettop(L);
    // check at least 1 arg
    if (argCount < 1) {
        return luaL_error(L, "Expected at least 1 argument");
    }

    // arg 1 should be boolean
    if (!lua_isnumber(L, 1)) {
        return luaL_error(L, "Bad argument #1 (expected int)");
    }

    executeImportedScript(luaL_checkinteger(L, 1));
    return 0;
}

int HSScript::lua_lagswitch(lua_State* L) {
    if (!s_instance) {
        lua_pushstring(L, "HSScript instance not available");
        lua_error(L);
        return 0;
    }
    int argCount = lua_gettop(L);
    // check at least 1 arg
    if (argCount < 1) {
        return luaL_error(L, "Expected at least 1 argument");
    }

    // arg 1 should be boolean
    if (!lua_isboolean(L, 1)) {
        return luaL_error(L, "Bad argument #1 (expected boolean)");
    }
    bool enable = lua_toboolean(L, 1);

    // second arg, number
    int pckt_loss = 0;
    if (argCount >= 2) {
        if (!lua_isnumber(L, 2)) {
            return luaL_error(L, "Bad argument #2 (expected number)");
        }
        pckt_loss = (int)lua_tointeger(L, 2);
    }

    int ping_lag = 0;
    if (argCount >= 3) {
        if (!lua_isnumber(L, 3)) {
            return luaL_error(L, "Bad argument #3 (expected number)");
        }
        ping_lag = (int)lua_tointeger(L, 3);
    }

    if (enable) {
        ctrl.lag(ping_lag, static_cast<double>(pckt_loss));
    } else {
        ctrl.disable();
    }

    return 0;
}

int HSScript::lua_pressKey(lua_State* L) {
    if (!s_instance) {
        lua_pushstring(L, "HSScript instance not available");
        lua_error(L);
        return 0;
    }

    const char* keyName = luaL_checkstring(L, 1);
    CrossInput::Key key = stringToKey(keyName);

    if (key == CrossInput::Key(0)) {
        std::string error = "Unknown key: " + std::string(keyName);
        lua_pushstring(L, error.c_str());
        lua_error(L);
        return 0;
    }

    // Optional delay parameter (default 50ms)
    int delay = 50;
    if (lua_gettop(L) >= 2) {
        delay = luaL_checkinteger(L, 2);
    }

    s_instance->m_input.pressKey(key, delay);
    return 0;
}

int HSScript::lua_holdKey(lua_State* L) {
    if (!s_instance) {
        lua_pushstring(L, "HSScript instance not available");
        lua_error(L);
        return 0;
    }

    const char* keyName = luaL_checkstring(L, 1);
    CrossInput::Key key = stringToKey(keyName);

    if (key == CrossInput::Key(0)) {
        std::string error = "Unknown key: " + std::string(keyName);
        lua_pushstring(L, error.c_str());
        lua_error(L);
        return 0;
    }

    s_instance->m_input.holdKey(key);
    return 0;
}

int HSScript::lua_releaseKey(lua_State* L) {
    if (!s_instance) {
        lua_pushstring(L, "HSScript instance not available");
        lua_error(L);
        return 0;
    }

    const char* keyName = luaL_checkstring(L, 1);
    CrossInput::Key key = stringToKey(keyName);

    if (key == CrossInput::Key(0)) {
        std::string error = "Unknown key: " + std::string(keyName);
        lua_pushstring(L, error.c_str());
        lua_error(L);
        return 0;
    }

    s_instance->m_input.releaseKey(key);
    return 0;
}

int HSScript::lua_isKeyPressed(lua_State* L) {
    if (!s_instance) {
        lua_pushstring(L, "HSScript instance not available");
        lua_error(L);
        return 0;
    }

    const char* keyName = luaL_checkstring(L, 1);
    CrossInput::Key key = stringToKey(keyName);

    if (key == CrossInput::Key(0)) {
        std::string error = "Unknown key: " + std::string(keyName);
        lua_pushstring(L, error.c_str());
        lua_error(L);
        return 0;
    }

    bool pressed = s_instance->m_input.isKeyPressed(key);
    lua_pushboolean(L, pressed);
    return 1; // Return the boolean value
}

int HSScript::lua_typeText(lua_State* L) {
    if (!s_instance) {
        lua_pushstring(L, "HSScript instance not available");
        lua_error(L);
        return 0;
    }

    const char* text = luaL_checkstring(L, 1);

    // Optional delay parameter (default 30ms)
    int delay = 30;
    if (lua_gettop(L) >= 2) {
        delay = luaL_checkinteger(L, 2);
    }

    s_instance->m_input.typeText(text, delay);
    return 0;
}

int HSScript::lua_moveMouse(lua_State* L) {
    if (!s_instance) {
        lua_pushstring(L, "HSScript instance not available");
        lua_error(L);
        return 0;
    }

    int dx = luaL_checkinteger(L, 1);
    int dy = luaL_checkinteger(L, 2);

    s_instance->m_input.moveMouse(dx, dy);
    return 0;
}
int HSScript::lua_turnDegrees(lua_State* L) {
    if (!s_instance) {
        lua_pushstring(L, "HSScript instance not available");
        lua_error(L);
        return 0;
    }

    int dx = luaL_checkinteger(L, 1);
    float wcantfindmebase_value = cam_fix_active ? dx * 2 * 1.388888889 : dx * 2;
    float wcantfindmemultiplier = 1; // Slight adjustment for accuracy

    float wcantfindmespeed_pixels_x = static_cast<int>(std::round((wcantfindmebase_value / roblox_sensitivity) * wcantfindmemultiplier));

    s_instance->m_input.moveMouse(wcantfindmespeed_pixels_x, 0);
    return 0;
}

int HSScript::lua_sleep(lua_State* L) {
    int ms = luaL_checkinteger(L, 1);
#if defined(_WIN32)
    Sleep(ms);          // Windows Sleep in milliseconds
#else
    usleep(ms * 1000);  // Linux usleep takes microseconds
#endif
       return 0;
}

int HSScript::lua_log(lua_State* L) {
    log(luaL_checkstring(L, 1));
    return 0;
}

int HSScript::lua_waitForKey(lua_State* L) {
    if (!s_instance) {
        lua_pushstring(L, "HSScript instance not available");
        lua_error(L);
        return 0;
    }

    // Optional timeout parameter (default -1 = wait indefinitely)
    int timeout = -1;
    if (lua_gettop(L) >= 1) {
        timeout = luaL_checkinteger(L, 1);
    }

    CrossInput::Key pressedKey = s_instance->m_input.getCurrentPressedKey(timeout);

    if (pressedKey == CrossInput::Key(0)) {
        lua_pushnil(L);
    } else {
        std::string keyName = s_instance->m_input.getKeyName(pressedKey);
        lua_pushstring(L, keyName.c_str());
    }

    return 1; // Return the key name or nil
}

int HSScript::lua_getKeyName(lua_State* L) {
    if (!s_instance) {
        lua_pushstring(L, "HSScript instance not available");
        lua_error(L);
        return 0;
    }

    const char* keyName = luaL_checkstring(L, 1);
    CrossInput::Key key = stringToKey(keyName);

    if (key == CrossInput::Key(0)) {
        lua_pushstring(L, "Unknown");
    } else {
        std::string name = s_instance->m_input.getKeyName(key);
        lua_pushstring(L, name.c_str());
    }

    return 1;
}

// ==================== HELPER FUNCTIONS ====================

CrossInput::Key HSScript::stringToKey(const std::string& keyName) {
    static std::unordered_map<std::string, CrossInput::Key> keyMap = {
        // Letters
        {"A", CrossInput::Key::A}, {"B", CrossInput::Key::B}, {"C", CrossInput::Key::C},
        {"D", CrossInput::Key::D}, {"E", CrossInput::Key::E}, {"F", CrossInput::Key::F},
        {"G", CrossInput::Key::G}, {"H", CrossInput::Key::H}, {"I", CrossInput::Key::I},
        {"J", CrossInput::Key::J}, {"K", CrossInput::Key::K}, {"L", CrossInput::Key::L},
        {"M", CrossInput::Key::M}, {"N", CrossInput::Key::N}, {"O", CrossInput::Key::O},
        {"P", CrossInput::Key::P}, {"Q", CrossInput::Key::Q}, {"R", CrossInput::Key::R},
        {"S", CrossInput::Key::S}, {"T", CrossInput::Key::T}, {"U", CrossInput::Key::U},
        {"V", CrossInput::Key::V}, {"W", CrossInput::Key::W}, {"X", CrossInput::Key::X},
        {"Y", CrossInput::Key::Y}, {"Z", CrossInput::Key::Z},

        // Numbers
        {"0", CrossInput::Key::Num0}, {"1", CrossInput::Key::Num1},
        {"2", CrossInput::Key::Num2}, {"3", CrossInput::Key::Num3},
        {"4", CrossInput::Key::Num4}, {"5", CrossInput::Key::Num5},
        {"6", CrossInput::Key::Num6}, {"7", CrossInput::Key::Num7},
        {"8", CrossInput::Key::Num8}, {"9", CrossInput::Key::Num9},

        // Function keys
        {"F1", CrossInput::Key::F1}, {"F2", CrossInput::Key::F2},
        {"F3", CrossInput::Key::F3}, {"F4", CrossInput::Key::F4},
        {"F5", CrossInput::Key::F5}, {"F6", CrossInput::Key::F6},
        {"F7", CrossInput::Key::F7}, {"F8", CrossInput::Key::F8},
        {"F9", CrossInput::Key::F9}, {"F10", CrossInput::Key::F10},
        {"F11", CrossInput::Key::F11}, {"F12", CrossInput::Key::F12},

        // Special keys
        {"Space", CrossInput::Key::Space}, {"Enter", CrossInput::Key::Enter},
        {"Tab", CrossInput::Key::Tab}, {"Escape", CrossInput::Key::Escape},
        {"Backspace", CrossInput::Key::Backspace}, {"Delete", CrossInput::Key::Delete},
        {"Insert", CrossInput::Key::Insert},

        // Arrow keys
        {"Left", CrossInput::Key::Left}, {"Right", CrossInput::Key::Right},
        {"Up", CrossInput::Key::Up}, {"Down", CrossInput::Key::Down},

        // Modifiers
        {"LShift", CrossInput::Key::LShift}, {"RShift", CrossInput::Key::RShift},
        {"LCtrl", CrossInput::Key::LCtrl}, {"RCtrl", CrossInput::Key::RCtrl},
        {"LAlt", CrossInput::Key::LAlt}, {"RAlt", CrossInput::Key::RAlt},

        // Mouse buttons
        {"LMB", CrossInput::Key::LMB}, {"RMB", CrossInput::Key::RMB},
        {"MMB", CrossInput::Key::MMB}, {"Mouse4", CrossInput::Key::Mouse4},
        {"Mouse5", CrossInput::Key::Mouse5},

        // Navigation
        {"Home", CrossInput::Key::Home}, {"End", CrossInput::Key::End},
        {"PageUp", CrossInput::Key::PageUp}, {"PageDown", CrossInput::Key::PageDown},

        // Numpad
        {"Numpad0", CrossInput::Key::Numpad0}, {"Numpad1", CrossInput::Key::Numpad1},
        {"Numpad2", CrossInput::Key::Numpad2}, {"Numpad3", CrossInput::Key::Numpad3},
        {"Numpad4", CrossInput::Key::Numpad4}, {"Numpad5", CrossInput::Key::Numpad5},
        {"Numpad6", CrossInput::Key::Numpad6}, {"Numpad7", CrossInput::Key::Numpad7},
        {"Numpad8", CrossInput::Key::Numpad8}, {"Numpad9", CrossInput::Key::Numpad9},

        // Lock keys
        {"CapsLock", CrossInput::Key::CapsLock}, {"NumLock", CrossInput::Key::NumLock},
        {"ScrollLock", CrossInput::Key::ScrollLock},

        // Brackets and symbols
        {"[", CrossInput::Key::LeftBracket}, {"]", CrossInput::Key::RightBracket},
        {"/", CrossInput::Key::Slash}, {";", CrossInput::Key::Semicolon},
        {"-", CrossInput::Key::Minus}, {"=", CrossInput::Key::Equal},
        {"\\", CrossInput::Key::Backslash}, {"'", CrossInput::Key::Quote},
        {",", CrossInput::Key::Comma}, {".", CrossInput::Key::Dot},
        {"`", CrossInput::Key::Grave},
    };

    auto it = keyMap.find(keyName);
    if (it != keyMap.end()) {
        return it->second;
    }

    return CrossInput::Key(0); // Unknown key
}

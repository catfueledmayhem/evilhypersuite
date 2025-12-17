/*
===============================================================================
HSScript - HyperSuite Scripting Engine
===============================================================================

A simple and tuff Lua scripting wrapper with full inpctrl (CrossInput) integration.
Allows loading and executing .hss (HyperSuite Script) files.

*/

#ifndef HSSCRIPT_HPP
#define HSSCRIPT_HPP

#include <string>
#include <functional>
#include "inpctrl.hpp"

extern "C" {
    #include "lua.h"
    #include "lualib.h"
    #include "lauxlib.h"
}

class HSScript {
public:
    HSScript();
    ~HSScript();

    // Initialize the scripting engine
    bool init();

    // Clean up resources
    void cleanup();

    // Load a .hss script file
    bool loadFile(const std::string& filename);

    // Execute Lua code directly
    bool executeString(const std::string& code);

    // Execute a Lua function by name (no arguments)
    bool executeFunction(const std::string& funcName);

    // Execute a Lua function with string argument
    bool executeFunctionWithString(const std::string& funcName, const std::string& arg);

    // Execute a Lua function with number argument
    bool executeFunctionWithNumber(const std::string& funcName, double arg);

    // Check if a function exists in the loaded script
    bool functionExists(const std::string& funcName);

    // Set a global variable (number)
    void setGlobalNumber(const std::string& varName, double value);

    // Set a global variable (string)
    void setGlobalString(const std::string& varName, const std::string& value);

    // Get a global variable (number)
    double getGlobalNumber(const std::string& varName);

    // Get a global variable (string)
    std::string getGlobalString(const std::string& varName);

    // Register a custom C++ function to be called from Lua
    void registerFunction(const std::string& name, lua_CFunction func);

    // Get the raw Lua state (for advanced usage)
    lua_State* getLuaState() { return L; }

    // Get the CrossInput instance (for advanced usage)
    CrossInput* getInput() { return &m_input; }

    static CrossInput::Key stringToKey(const std::string& keyName);
private:
    lua_State* L;
    CrossInput m_input;
    bool m_initialized;

    // Register all CrossInput functions with Lua
    void registerInputFunctions();

    // Static callback functions for Lua
    static int lua_clear(lua_State* L);
    static int lua_pressKey(lua_State* L);
    static int lua_holdKey(lua_State* L);
    static int lua_releaseKey(lua_State* L);
    static int lua_isKeyPressed(lua_State* L);
    static int lua_typeText(lua_State* L);
    static int lua_moveMouse(lua_State* L);
    static int lua_sleep(lua_State* L);
    static int lua_log(lua_State* L);
    static int lua_waitForKey(lua_State* L);
    static int lua_getKeyName(lua_State* L);
    static int lua_lagswitch(lua_State* L);
    static int lua_roblox_freeze(lua_State* L);

    // Helper function to convert string to Key enum
    //static CrossInput::Key stringToKey(const std::string& keyName);

    // Global instance pointer for callbacks
    static HSScript* s_instance;
};

#endif // HSSCRIPT_HPP

#pragma once
#include "Globals.hpp"
#include "logzz.hpp"
#include "hsscript.hpp"
#include "Helper.hpp"
#include <string>
#include <vector>
#include <filesystem>
#include <fstream>
#include <memory>

struct ImportedScript {
    std::string name;
    std::string desc;
    std::string author;
    std::string version;
    std::string keybind;
    std::string filePath;
    int scriptIndex;
    bool enabled;

    ImportedScript() : scriptIndex(-1), enabled(false) {}
};

inline std::vector<std::unique_ptr<HSScript>> HSScriptClasses;
inline std::vector<ImportedScript> ImportedMacros;
inline std::string currentImportedOption = "";
inline int pendingKeybindScriptIndex = -1;

inline void initLuaEnvValues(HSScript& script) {
    script.setGlobalString("HSPlayerName", logzz::current_username.empty() ? "player" : logzz::current_username);
    script.setGlobalString("HSPlayerDisplayName", logzz::current_display_name.empty() ? "player" : logzz::current_display_name);
}

inline ImportedScript parseScriptMetadata(const std::string& filePath) {
    ImportedScript meta;
    meta.filePath = filePath;
    meta.name = std::filesystem::path(filePath).stem().string();

    std::ifstream file(filePath);
    if (!file.is_open()) {
        return meta;
    }

    std::string line;
    while (std::getline(file, line)) {
        if (line.find("-- @name:") == 0) {
            meta.name = line.substr(9);
            meta.name.erase(0, meta.name.find_first_not_of(" \t"));
            meta.name.erase(meta.name.find_last_not_of(" \t\r\n") + 1);
        }
        else if (line.find("-- @desc:") == 0 || line.find("-- @description:") == 0) {
            size_t pos = line.find(':') + 1;
            meta.desc = line.substr(pos);
            meta.desc.erase(0, meta.desc.find_first_not_of(" \t"));
            meta.desc.erase(meta.desc.find_last_not_of(" \t\r\n") + 1);
        }
        else if (line.find("-- @author:") == 0) {
            meta.author = line.substr(11);
            meta.author.erase(0, meta.author.find_first_not_of(" \t"));
            meta.author.erase(meta.author.find_last_not_of(" \t\r\n") + 1);
        }
        else if (line.find("-- @version:") == 0) {
            meta.version = line.substr(12);
            meta.version.erase(0, meta.version.find_first_not_of(" \t"));
            meta.version.erase(meta.version.find_last_not_of(" \t\r\n") + 1);
        }
        else if (line.find("-- @keybind:") == 0) {
            meta.keybind = line.substr(12);
            meta.keybind.erase(0, meta.keybind.find_first_not_of(" \t"));
            meta.keybind.erase(meta.keybind.find_last_not_of(" \t\r\n") + 1);
        }
        else if (!line.empty() && line[0] != '-') {
            break;
        }
    }

    file.close();
    return meta;
}

inline bool importScriptFromSavedData(const std::string& filePath, const std::string& keybind, bool enabled) {
    if (!std::filesystem::exists(filePath)) {
        std::cerr << "[ScriptManager] Script file no longer exists: " << filePath << std::endl;
        return false;
    }

    for (const auto& existing : ImportedMacros) {
        if (existing.filePath == filePath) {
            return false;
        }
    }

    ImportedScript meta = parseScriptMetadata(filePath);
    meta.keybind = keybind;
    meta.enabled = enabled;

    auto script = std::make_unique<HSScript>();
    if (!script->init()) {
        std::cerr << "Failed to initialize script: " << filePath << std::endl;
        return false;
    }

    initLuaEnvValues(*script);

    if (!script->loadFile(filePath)) {
        std::cerr << "Failed to load script: " << filePath << std::endl;
        script->cleanup();
        return false;
    }

    meta.scriptIndex = static_cast<int>(HSScriptClasses.size());
    HSScriptClasses.push_back(std::move(script));
    ImportedMacros.push_back(meta);

    std::cout << "[ScriptManager] Restored: " << meta.name << std::endl;
    return true;
}

inline bool importScript(const std::string& filePath) {
    if (!std::filesystem::exists(filePath)) {
        std::cerr << "Script file not found: " << filePath << std::endl;
        return false;
    }

    ImportedScript meta = parseScriptMetadata(filePath);

    for (const auto& existing : ImportedMacros) {
        if (existing.filePath == filePath) {
            std::cerr << "Script already imported: " << filePath << std::endl;
            return false;
        }
    }

    auto script = std::make_unique<HSScript>();
    if (!script->init()) {
        std::cerr << "Failed to initialize script: " << filePath << std::endl;
        return false;
    }

    initLuaEnvValues(*script);

    if (!script->loadFile(filePath)) {
        std::cerr << "Failed to load script: " << filePath << std::endl;
        script->cleanup();
        return false;
    }

    if (script->functionExists("getMetadata")) {
        std::string luaName = script->getGlobalString("HSMacroName");
        std::string luaDesc = script->getGlobalString("HSMacroDesc");

        if (!luaName.empty() && luaName != "unset") {
            meta.name = luaName;
        }
        if (!luaDesc.empty() && luaDesc != "unset") {
            meta.desc = luaDesc;
        }
    }

    meta.scriptIndex = static_cast<int>(HSScriptClasses.size());
    meta.enabled = false;
    HSScriptClasses.push_back(std::move(script));
    ImportedMacros.push_back(meta);

    std::cout << "[ScriptManager] Imported: " << meta.name << std::endl;
    return true;
}

inline void importScriptsFromDirectory(const std::string& directory) {
    if (!std::filesystem::exists(directory)) {
        std::filesystem::create_directories(directory);
        std::cout << "[ScriptManager] Created scripts directory: " << directory << std::endl;
        return;
    }

    int importedCount = 0;
    for (const auto& entry : std::filesystem::directory_iterator(directory)) {
        if (entry.is_regular_file() && entry.path().extension() == ".hss") {
            if (importScript(entry.path().string())) {
                importedCount++;
            }
        }
    }

    std::cout << "[ScriptManager] Imported " << importedCount << " scripts from " << directory << std::endl;
}

inline bool isKeybindPressed(CrossInput& input, const std::string& keybind) {
    if (keybind.empty()) return false;

    std::vector<std::string> keys;
    std::string current;
    for (char c : keybind) {
        if (c == '+') {
            if (!current.empty()) {
                keys.push_back(current);
                current.clear();
            }
        } else {
            current += c;
        }
    }
    if (!current.empty()) {
        keys.push_back(current);
    }

    if (keys.empty()) return false;

    for (size_t i = 0; i < keys.size() - 1; i++) {
        CrossInput::Key key = HSScript::stringToKey(keys[i]);
        if (!input.isKeyPressed(key)) {
            return false;
        }
    }

    CrossInput::Key finalKey = HSScript::stringToKey(keys.back());
    return input.isKeyPressed(finalKey);
}

inline void updateImportedScripts(CrossInput& input) {
    static std::vector<bool> keyWasPressed(ImportedMacros.size(), false);

    if (keyWasPressed.size() != ImportedMacros.size()) {
        keyWasPressed.resize(ImportedMacros.size(), false);
    }

    for (size_t i = 0; i < ImportedMacros.size(); i++) {
        ImportedScript& macro = ImportedMacros[i];

        if (!macro.enabled || macro.scriptIndex < 0 || macro.keybind.empty()) {
            continue;
        }

        bool isPressed = isKeybindPressed(input, macro.keybind);

        if (isPressed && !keyWasPressed[i]) {
            HSScript* script = HSScriptClasses[macro.scriptIndex].get();

            if (script && script->functionExists("onExecute")) {
                script->executeFunction("onExecute");
            }
        }

        keyWasPressed[i] = isPressed;
    }
}

inline void HandleImportedScriptKeybindCapture(CrossInput& input) {
    if (pendingKeybindScriptIndex < 0) return;

    CrossInput::Key key = input.getCurrentPressedKey(0);
    if (key != CrossInput::Key(0)) {
        std::string keyName = input.getKeyName(key);
        if (static_cast<size_t>(pendingKeybindScriptIndex) < ImportedMacros.size()) {
            ImportedMacros[pendingKeybindScriptIndex].keybind = keyName;
        }
        pendingKeybindScriptIndex = -1;
    }
}

inline void executeImportedScript(size_t index) {
    if (index >= ImportedMacros.size()) {
        std::cerr << "Invalid script index: " << index << std::endl;
        return;
    }

    ImportedScript& macro = ImportedMacros[index];
    if (macro.scriptIndex < 0 || static_cast<size_t>(macro.scriptIndex) >= HSScriptClasses.size()) {
        std::cerr << "Script not properly loaded: " << macro.name << std::endl;
        return;
    }

    HSScript* script = HSScriptClasses[macro.scriptIndex].get();
    if (!script) {
        std::cerr << "Script is null: " << macro.name << std::endl;
        return;
    }

    if (script->functionExists("onExecute")) {
        script->executeFunction("onExecute");
    } else {
        std::cerr << "Script '" << macro.name << "' has no onExecute function" << std::endl;
    }
}

inline bool reloadScript(size_t index) {
    if (index >= ImportedMacros.size()) {
        return false;
    }

    ImportedScript& macro = ImportedMacros[index];
    if (macro.scriptIndex < 0 || static_cast<size_t>(macro.scriptIndex) >= HSScriptClasses.size()) {
        return false;
    }

    HSScript* script = HSScriptClasses[macro.scriptIndex].get();
    if (!script) {
        return false;
    }

    script->cleanup();

    if (!script->init()) {
        return false;
    }

    initLuaEnvValues(*script);
    return script->loadFile(macro.filePath);
}

inline void removeImportedScript(size_t index) {
    if (index >= ImportedMacros.size()) {
        return;
    }

    ImportedScript& macro = ImportedMacros[index];
    if (macro.scriptIndex >= 0 && static_cast<size_t>(macro.scriptIndex) < HSScriptClasses.size()) {
        HSScriptClasses[macro.scriptIndex]->cleanup();
        HSScriptClasses.erase(HSScriptClasses.begin() + macro.scriptIndex);
    }

    ImportedMacros.erase(ImportedMacros.begin() + index);

    for (size_t i = index; i < ImportedMacros.size(); i++) {
        if (ImportedMacros[i].scriptIndex > static_cast<int>(index)) {
            ImportedMacros[i].scriptIndex--;
        }
    }
}

inline void cleanupAllScripts() {
    for (auto& script : HSScriptClasses) {
        if (script) {
            script->cleanup();
        }
    }
    HSScriptClasses.clear();
    ImportedMacros.clear();
}

inline void initScriptSystem() {
    importScriptsFromDirectory("scripts");

    std::cout << "[ScriptManager] Script system initialized with "
              << ImportedMacros.size() << " scripts" << std::endl;
}

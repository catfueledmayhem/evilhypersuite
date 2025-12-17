#pragma once
#include "hsscriptman.hpp"
#include "imgui.h"

extern std::string current_option;

inline void DrawImportedScriptButton(size_t index, const ImVec4& themeColor) {
    if (index >= ImportedMacros.size()) return;

    ImportedScript& macro = ImportedMacros[index];
    std::string label = macro.name;
    std::string uniqueId = "imported_" + std::to_string(index);

    bool isEnabled = macro.enabled;

    if (isEnabled) {
        ImGui::PushStyleColor(ImGuiCol_Button, themeColor);
        ImGui::PushStyleColor(ImGuiCol_ButtonHovered, ImVec4(themeColor.x * 1.25f, themeColor.y * 2.0f, themeColor.z * 2.0f, 1.0f));
        ImGui::PushStyleColor(ImGuiCol_ButtonActive, ImVec4(themeColor.x * 0.7f, themeColor.y * 0.7f, themeColor.z * 0.7f, 1.0f));
    } else {
        ImGui::PushStyleColor(ImGuiCol_Button, ImVec4(themeColor.x * 0.44f, themeColor.y * 0.7f, themeColor.z * 0.7f, 1.0f));
        ImGui::PushStyleColor(ImGuiCol_ButtonHovered, ImVec4(themeColor.x * 0.56f, themeColor.y * 1.0f, themeColor.z * 1.0f, 1.0f));
        ImGui::PushStyleColor(ImGuiCol_ButtonActive, ImVec4(themeColor.x * 0.375f, themeColor.y * 0.5f, themeColor.z * 0.5f, 1.0f));
    }

    if (ImGui::Button((label + "##" + uniqueId).c_str(), ImVec2(-1, 20))) {
        if (currentImportedOption == uniqueId) {
            currentImportedOption = "";
        } else {
            currentImportedOption = uniqueId;
            current_option = "";
        }
    }

    ImGui::PopStyleColor(3);

    if (ImGui::IsItemHovered() && ImGui::IsMouseClicked(ImGuiMouseButton_Right)) {
        macro.enabled = !macro.enabled;
    }

    if (currentImportedOption == uniqueId) {
        float availableWidth = ImGui::GetContentRegionAvail().x;

        ImGui::BeginChild((uniqueId + "_frame").c_str(), ImVec2(availableWidth, 80), true);

        ImGui::TextWrapped("Enabled: %s", macro.enabled ? "Yes" : "No");
        ImGui::Separator();
        ImGui::Text("Keybind: %s", macro.keybind.empty() ? "None" : macro.keybind.c_str());

        if (pendingKeybindScriptIndex == static_cast<int>(index)) {
            ImGui::TextColored(ImVec4(1.0f, 1.0f, 0.0f, 1.0f), "Press a key...");
        } else {
            if (ImGui::Button("Change", ImVec2(-1, 20.0f))) {
                pendingKeybindScriptIndex = static_cast<int>(index);
            }
        }

        ImGui::EndChild();
    }
}

inline void DrawAllImportedScripts(const ImVec4& themeColor) {
    if (ImportedMacros.empty()) return;

    ImGui::Spacing();
    ImGui::Separator();
    ImGui::TextColored(ImVec4(0.7f, 0.7f, 0.7f, 1.0f), "Imported Scripts:");
    ImGui::Separator();
    ImGui::Spacing();

    for (size_t i = 0; i < ImportedMacros.size(); i++) {
        DrawImportedScriptButton(i, themeColor);
    }
}

inline void DrawImportedScriptRightPanel() {
    if (currentImportedOption.empty()) return;
    if (currentImportedOption.find("imported_") != 0) return;

    size_t index = std::stoul(currentImportedOption.substr(9));
    if (index >= ImportedMacros.size()) return;

    ImportedScript& macro = ImportedMacros[index];

    ImGui::Text("%s", macro.name.c_str());
    ImGui::Separator();

    if (!macro.desc.empty()) {
        ImGui::TextWrapped("%s", macro.desc.c_str());
    } else {
        ImGui::TextWrapped("No description provided for this script.");
    }

    ImGui::Spacing();

    if (!macro.author.empty()) {
        ImGui::Text("Author: %s", macro.author.c_str());
    }
    if (!macro.version.empty()) {
        ImGui::Text("Version: %s", macro.version.c_str());
    }

    ImGui::Spacing();
    ImGui::Separator();
    ImGui::Spacing();

    ImGui::TextColored(ImVec4(0.7f, 0.7f, 0.7f, 1.0f), "File: %s", macro.filePath.c_str());

    ImGui::Spacing();

    if (ImGui::Button("Execute Now", ImVec2(100, 20))) {
        executeImportedScript(index);
    }

    if (ImGui::Button("Reload Script", ImVec2(100, 20))) {
        if (reloadScript(index)) {
            std::cout << "[ScriptManager] Reloaded: " << macro.name << std::endl;
        }
    }

    ImGui::PushStyleColor(ImGuiCol_Button, ImVec4(0.8f, 0.2f, 0.2f, 1.0f));
    ImGui::PushStyleColor(ImGuiCol_ButtonHovered, ImVec4(1.0f, 0.3f, 0.3f, 1.0f));
    ImGui::PushStyleColor(ImGuiCol_ButtonActive, ImVec4(0.6f, 0.1f, 0.1f, 1.0f));

    if (ImGui::Button("Remove", ImVec2(100, 20))) {
        removeImportedScript(index);
        currentImportedOption = "";
    }

    ImGui::PopStyleColor(3);
}

inline void ClearImportedOptionOnBuiltinSelect() {
    currentImportedOption = "";
}

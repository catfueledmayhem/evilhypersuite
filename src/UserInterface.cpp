#include "UserInterface.hpp"
#include "raylib.h"
#include "LoadTextures.hpp"
#include "imgui.h"
#include "rlImGui.h"
#include "ImGuiFileDialog.h"
#include "FPSDropper.hpp"
#include "inpctrl.hpp"
#include "Globals.hpp"
#include "logzz.hpp"
#include "Helper.hpp"
#include "LagSwitch.hpp"
#include "Speedglitch.hpp"
#include "WallhopAndWallwalk.hpp"
#include "GlobalBasicSettings.hpp"
#include "hsscript.hpp"
#include "ServerHop.hpp"
#include "hsscriptman.hpp"
#include "ImportedScriptsUI.hpp"
#include <string>

ImVec4 orange = ImVec4(1.0f, 0.55f, 0.1f, 1.0f);

ImVec4 HSVtoRGB(float h, float s, float v) {
    float r, g, b;

    int i = (int)(h * 6.0f);
    float f = h * 6.0f - i;
    float p = v * (1.0f - s);
    float q = v * (1.0f - f * s);
    float t = v * (1.0f - (1.0f - f) * s);

    switch (i % 6) {
        case 0: r = v; g = t; b = p; break;
        case 1: r = q; g = v; b = p; break;
        case 2: r = p; g = v; b = t; break;
        case 3: r = p; g = q; b = v; break;
        case 4: r = t; g = p; b = v; break;
        case 5: r = v; g = p; b = q; break;
    }

    return ImVec4(r, g, b, 1.0f);
}

void applyThemeColor(const ImVec4& color) {
    ImGuiStyle& style = ImGui::GetStyle();

    // --- Buttons ---
    style.Colors[ImGuiCol_Button] = color;
    style.Colors[ImGuiCol_ButtonHovered] = ImVec4(color.x * 1.25f, color.y * 2.0f, color.z * 2.0f, 1.0f);
    style.Colors[ImGuiCol_ButtonActive] = ImVec4(color.x * 0.7f, color.y * 0.7f, color.z * 0.7f, 1.0f);

    // --- Tabs ---
    style.Colors[ImGuiCol_Tab] = color;
    style.Colors[ImGuiCol_TabHovered] = ImVec4(color.x * 1.25f, color.y * 2.0f, color.z * 2.0f, 1.0f);
    style.Colors[ImGuiCol_TabActive] = ImVec4(color.x * 1.25f, color.y * 3.0f, color.z * 3.0f, 1.0f);
    style.Colors[ImGuiCol_TabUnfocused] = ImVec4(color.x * 0.75f, color.y * 1.0f, color.z * 1.0f, 1.0f);
    style.Colors[ImGuiCol_TabUnfocusedActive] = color;
    style.TabRounding = 0.0f;

    // --- Checkboxes ---
    style.Colors[ImGuiCol_CheckMark] = ImVec4(color.x * 1.25f, color.y * 2.0f, color.z * 2.0f, 1.0f);
    style.Colors[ImGuiCol_FrameBg] = ImVec4(color.x * 0.75f, color.y * 1.0f, color.z * 1.0f, 1.0f);
    style.Colors[ImGuiCol_FrameBgHovered] = color;
    style.Colors[ImGuiCol_FrameBgActive] = ImVec4(color.x * 1.25f, color.y * 2.0f, color.z * 2.0f, 1.0f);

    // --- Window title bars ---
    style.Colors[ImGuiCol_TitleBg] = ImVec4(color.x * 0.8f, color.y * 0.8f, color.z * 0.8f, 1.0f);        // idle
    style.Colors[ImGuiCol_TitleBgActive] = ImVec4(color.x, color.y, color.z, 1.0f);                        // active/focused
    style.Colors[ImGuiCol_TitleBgCollapsed] = ImVec4(color.x * 0.5f, color.y * 0.5f, color.z * 0.5f, 1.0f); // collapsed

    // --- Sliders ---
    style.Colors[ImGuiCol_SliderGrab] = ImVec4(color.x * 1.25f, color.y * 2.0f, color.z * 2.0f, 1.0f);
    style.Colors[ImGuiCol_SliderGrabActive] = ImVec4(color.x * 1.25f, color.y * 3.0f, color.z * 3.0f, 1.0f);
    style.GrabMinSize = 8.0f;

    // --- Scrollbars ---
    style.Colors[ImGuiCol_ScrollbarBg] = ImVec4(color.x * 0.25f, color.y * 0.5f, color.z * 0.5f, 1.0f);
    style.Colors[ImGuiCol_ScrollbarGrab] = color;
    style.Colors[ImGuiCol_ScrollbarGrabHovered] = ImVec4(color.x * 1.25f, color.y * 2.0f, color.z * 2.0f, 1.0f);
    style.Colors[ImGuiCol_ScrollbarGrabActive] = ImVec4(color.x * 1.25f, color.y * 3.0f, color.z * 3.0f, 1.0f);

    // --- Combo / selectable items ---
    style.Colors[ImGuiCol_Header] = color;
    style.Colors[ImGuiCol_HeaderHovered] = ImVec4(color.x * 1.25f, color.y * 2.0f, color.z * 2.0f, 1.0f);
    style.Colors[ImGuiCol_HeaderActive] = ImVec4(color.x * 0.875f, color.y * 0.0f, color.z * 0.0f, 1.0f);

    style.ScrollbarRounding = 0.0f;
    style.ScrollbarSize = 12.0f;
}

void initUI() {
    // Initialize rlImGui
    rlImGuiSetup(true);
    applyThemeColor(themeColor);
    if (first_time) {
        strncpy(setup_wizard_temp_process_name, roblox_process_name.c_str(), sizeof(setup_wizard_temp_process_name));
        setup_wizard_temp_process_name[sizeof(setup_wizard_temp_process_name)-1] = '\0';
    }

}

void UpdateUI() {
    //rainbow theme
    if (rainbowThemeEnabled) {
        // Update hue
        rainbowHue += rainbowSpeed;
        if (rainbowHue > 1.0f) rainbowHue = 0.0f;

        // Convert HSV -> RGB
        themeColor = HSVtoRGB(rainbowHue, rainbowSaturation, rainbowValue);
        applyThemeColor(themeColor);
    }
    float w = std::max(screen_width, 1.0f);
    float h = std::max(screen_height, 1.0f);

    ImGuiIO& io = ImGui::GetIO();
    io.DisplaySize = ImVec2(w, h);
    ImGui::SetNextWindowPos(ImVec2(0, 0), ImGuiCond_Always);
    ImGui::SetNextWindowSize(ImVec2(w, h), ImGuiCond_Always);

    ImGui::Begin("Roblox Hypersuite", nullptr,
                ImGuiWindowFlags_NoTitleBar |
                ImGuiWindowFlags_NoResize |
                ImGuiWindowFlags_NoMove |
                ImGuiWindowFlags_NoCollapse);

    if (is_elevated && !first_time) {
        switch (logzz::current_state) {
            case OFFLINE: {
                ImGui::Text("Roblox Hypersuite - Currently not on roblox.");
                break;
            }
            case IN_LUA_APP: {
                ImGui::Text("Roblox Hypersuite - Currently in the lua app");
                break;
            }
            case IN_GAME: {
                std::string placeName = logzz::find_name_for_universe(logzz::current_universe_ID);
                std::string text;
                if (placeName.empty()) {
                    text = "Roblox Hypersuite - Currently in game";
                } else {
                    text = "Roblox Hypersuite - Currently in game (" + placeName + ")";
                }
                ImGui::Text("%s", text.c_str());
                break;
            }
            default: {
                ImGui::Text("Roblox Hypersuite");
                break;
            }
        }
    } else {
        ImGui::Text("Roblox Hypersuite");
    }

    ImGui::Separator();
    if (!is_elevated) {
#ifdef _WIN32
        ImGui::TextWrapped(
            "This program needs administrator privileges to run.\n"
            "Please run this program as administrator."
        );
        if (ImGui::Button("Exit", ImVec2(80.0f, 20.0f))) {
            exit(0);
        }
        ImGui::End();
        return;
#else
        ImGui::TextWrapped(
            "This program needs administrator privileges to run.\n"
            "Please enter your password to elevate:"
        );

        ImGui::Spacing();

        ImGui::InputText("##password", passwordBuffer, sizeof(passwordBuffer),
                        ImGuiInputTextFlags_Password);

        ImGui::Spacing();

        if (ImGui::Button("Elevate", ImVec2(80.0f, 20.0f)))
        {
            if (!TryElevate(passwordBuffer))
                elevationFailed = true;
        }

        if (elevationFailed)
        {
            ImGui::Spacing();
            ImGui::TextColored(
                ImVec4(1.0f, 0.2f, 0.2f, 1.0f),
                "Elevation failed. Incorrect password?"
            );
        }

        ImGui::End();
        return;
#endif
    }

    //--------------FIRST TIME SETUP WIZARD THINGY------------------
    if (first_time) {
        switch (setup_wizard_page) {
            case 0:
                ImGui::TextWrapped(
                    "Hello! Welcome to roblox hypersuite setup wizard!.\n\nThis is a roblox chea- I mean a roblox utility. With many features and macros to enhance your glitching/obbying experience. "
                    "Let's get you set up with some basic configuration."
                );
                ImGui::Spacing();
                ImGui::Separator();
                ImGui::Spacing();
                ImGui::TextColored(orange, "Features include:");
                ImGui::BulletText("Speedglitch - Gain massive velocity while airborne");
                ImGui::BulletText("Helicopter High Jump - Launch yourself upward");
                ImGui::BulletText("Wallhop - Automatically time wallhops");
                ImGui::BulletText("Wallwalk - Walk across walls without jumping");
                ImGui::BulletText("Lag switch - Stop roblox packets from being sent.");
                ImGui::BulletText("Ping increaser - Increases your ping for some glitches.");
                ImGui::BulletText("Various clip techniques");
                ImGui::BulletText("And much more!");
                break;
            case 1:
                ImGui::TextWrapped("Step 1: Roblox Process Configuration");
                ImGui::Separator();
                ImGui::Spacing();
                ImGui::TextWrapped(
                    "Enter the name of your Roblox executable. This allows the macro to detect "
                    "when Roblox is running and apply certain features."
                );

                ImGui::Spacing();
                ImGui::Text("Roblox Process Name:");
                ImGui::SetNextItemWidth(-1);
                ImGui::InputText("##wizard_process", setup_wizard_temp_process_name, sizeof(setup_wizard_temp_process_name));
                ImGui::Spacing();
                ImGui::TextWrapped("Common values:");
                ImGui::BulletText("Windows: RobloxPlayerBeta.exe");
                ImGui::BulletText("Linux (Grapejuice): grapejuice");
                ImGui::BulletText("Linux (Vinegar): vinegar");
                ImGui::BulletText("Linux (Sober): sober / sober.real");

                ImGui::Spacing();
                ImGui::TextColored(ImVec4(0.7f, 0.7f, 0.7f, 1.0f),
                    "You can change this later in settings.");
                break;
            case 2: { // Keyboard layout
                ImGui::TextWrapped("Step 2: Keyboard Layout");
                ImGui::Separator();
                ImGui::Spacing();

                ImGui::TextWrapped(
                    "Select your keyboard layout. This ensures chat commands are typed correctly."
                );

                ImGui::Spacing();
                ImGui::Text("Keyboard Layout:");
                ImGui::PushStyleVar(ImGuiStyleVar_FramePadding, ImVec2(2.0f, 1.0f));
                ImGui::PushStyleVar(ImGuiStyleVar_ItemSpacing, ImVec2(6.0f, 4.0f));

                ImGui::RadioButton("QWERTY (US/UK Standard)", &temp_kb_layout, 0);
                ImGui::RadioButton("AZERTY (French)", &temp_kb_layout, 1);

                ImGui::PopStyleVar(2);

                ImGui::Spacing();
                ImGui::TextWrapped(
                    "QWERTY is the most common layout. Choose AZERTY if you use a French keyboard."
                );
                break;
            }
            case 3: {
                ImGui::TextWrapped("Step 3: Roblox Game Settings");
                ImGui::Separator();
                ImGui::Spacing();

                ImGui::TextWrapped(
                    "Configure your Roblox in-game settings. These are critical for macros "
                    "that involve camera movement."
                );

                ImGui::Spacing();

                ImGui::Text("Roblox Camera Sensitivity (0.1 - 4.0):");
                ImGui::SetNextItemWidth(200);
                ImGui::SliderFloat("##wizard_sens", &temp_sensitivity, 0.1f, 4.0f, "%.2f");
                if (temp_sensitivity < 0.001f) temp_sensitivity = 1.0f;
                ImGui::SameLine();
                ImGui::TextDisabled("(?)");
                if (ImGui::IsItemHovered()) {
                    ImGui::SetTooltip("Found in Roblox Settings > Camera > Camera Sensitivity");
                }

                ImGui::Spacing();

                ImGui::Text("Your Roblox FPS:");
                ImGui::SetNextItemWidth(200);
                ImGui::InputInt("##wizard_fps", &temp_fps);
                if (temp_fps < 1) temp_fps = 60;
                ImGui::SameLine();
                ImGui::TextDisabled("(?)");
                if (ImGui::IsItemHovered()) {
                    ImGui::SetTooltip("Press Shift+F5 in Roblox to see your FPS");
                }
                ImGui::Spacing();
                ImGui::TextColored(ImVec4(0.7f, 0.7f, 0.7f, 1.0f), "These settings can be adjusted anytime in the main interface.");
                break;
            }
            case 4: {
                ImGui::TextWrapped("Step 4: Ready to Go!");
                ImGui::Separator();
                ImGui::Spacing();

                ImGui::TextWrapped("Your configuration summary:");

                ImGui::Spacing();
                ImGui::BulletText("Process Name: %s", setup_wizard_temp_process_name);
                ImGui::BulletText("Keyboard Layout: %s",
                    temp_kb_layout == 0 ? "QWERTY" : "AZERTY");
                ImGui::BulletText("Sensitivity: %.2f", temp_sensitivity);
                ImGui::BulletText("Target FPS: %d", temp_fps);

                ImGui::Spacing();
                ImGui::Separator();
                ImGui::Spacing();

                ImGui::TextWrapped("Tips for getting started:");
                ImGui::BulletText("Check the keybinds section to see default hotkeys");
                ImGui::BulletText("Enable/disable macros in the main interface");
                ImGui::BulletText("Most macros require COM offset. Get it with gear desync.");
                ImGui::BulletText("Read the docs at https://3443o-o.github.io/hypersuite/");

                ImGui::Spacing();
                ImGui::Spacing();

                ImGui::TextColored(ImVec4(0.0f, 1.0f, 0.5f, 1.0f),
                    "Click 'Finish' to start using the macro utility!");
                break;
            }
        }

        ImVec2 button_size(80.0f, 20.0f);
        float padding = 10.0f;

        ImVec2 window_size = ImGui::GetWindowSize();

        if (setup_wizard_page == 0) {
            /* Bottom-left */
            ImGui::SetCursorPos(ImVec2(
                padding,
                window_size.y - button_size.y - padding
            ));

            if (ImGui::Button("Skip Setup", button_size)) {
                // Mark setup as skipped / complete
                setup_complete = true;
                first_time = false;

                log("Setup wizard skipped by user");
            }
        }

        ImGui::SetCursorPos(ImVec2(
            window_size.x - (button_size.x * 2) - padding - 8.0f,
            window_size.y - button_size.y - padding
        ));
        ImGui::BeginDisabled(setup_wizard_page == 0 || setup_wizard_page >= 4);
        if (ImGui::Button("Previous", button_size)) {
            setup_wizard_page--;
        }
        ImGui::EndDisabled();

        ImGui::SameLine();
        if (setup_wizard_page >= 4) {
            if (ImGui::Button("Finish", button_size)) {
                // Apply all settings
                roblox_process_name = setup_wizard_temp_process_name;
                kb_layout = temp_kb_layout;
                roblox_sensitivity = temp_sensitivity;
                roblox_fps = temp_fps;

                // Update dependent calculations
                updateSpeedglitchSensitivity(roblox_sensitivity, cam_fix_active);
                updateSpeedglitchFPS(roblox_fps);
                calculateWallhopPixels();
                calculateWallwalkPixels();

                // Mark setup as complete
                setup_complete = true;
                first_time = false;

                log("Setup wizard completed successfully");
            }
        } else  {
            if (setup_wizard_page == 1) {
                bool is_empty = (strlen(setup_wizard_temp_process_name) == 0);

                ImGui::BeginDisabled(is_empty || setup_wizard_page == 0);
                if (ImGui::Button("Next", button_size)) {
                    setup_wizard_page++;
                }
                ImGui::EndDisabled();
            } else {
                if (ImGui::Button("Next", button_size)) {
                    setup_wizard_page++;
                }
            }

        }

        ImGui::End();
        return;
    }
    // Tab Bar
    if (ImGui::BeginTabBar("MainTabBar")) {

        // MACRO TAB
        if (ImGui::BeginTabItem("Macros")) {

            // ---------- LAYOUT ----------
            ImVec2 window_size = ImGui::GetContentRegionAvail();
            float left_width = 200.0f;

            // LEFT PANEL: scrollable buttons
            ImGui::BeginChild("Left Panel", ImVec2(left_width, window_size.y), true, ImGuiWindowFlags_AlwaysVerticalScrollbar);

            // Helper lambda for buttons with expandable frame
            auto DrawOptionButton = [&](const char* label) {
                std::string CodeName;

                if (std::string(label) == "Freeze") {
                    CodeName = "Freeze";
                } else if (std::string(label) == "Extended Laugh Clip") {
                    CodeName = "Laugh";
                } else if (std::string(label) == "Extended Dance Clip") {
                    CodeName = "E-Dance";
                }  else if (std::string(label) == "Buckey clip") {
                    CodeName = "Buckey-clip";
                } else if (std::string(label) == "Speed glitch") {
                    CodeName = "Speedglitch";
                } else if (std::string(label) == "Spam key") {
                    CodeName = "Spam-Key";
                } else if (std::string(label) == "Disable head collision") {
                    CodeName = "Disable-Head-Collision";
                } else if (std::string(label) == "NHC Roof Clip") {
                    CodeName = "NHC-Roof";
                } else if (std::string(label) == "Helicopter High Jump") {
                    CodeName = "HHJ";
                } else if (std::string(label) == "Full Gear Desync") {
                    CodeName = "Full-Gear-Desync";
                } else if (std::string(label) == "Floor Bounce High Jump") {
                    CodeName = "Floor-Bounce-High-Jump";
                } else if (std::string(label) == "FPS Dropper") {
                    CodeName = "FPS-Drop";
                } else {
                    CodeName = std::string(label);
                }

                // Determine if this option is enabled
                bool isEnabled = enabled[GetIDFromCodeName(CodeName)];

                // Set button colors based on enabled state
                if (isEnabled) {
                    ImGui::PushStyleColor(ImGuiCol_Button, themeColor);
                    ImGui::PushStyleColor(ImGuiCol_ButtonHovered, ImVec4(themeColor.x * 1.25f, themeColor.y * 2.0f, themeColor.z * 2.0f, 1.0f));
                    ImGui::PushStyleColor(ImGuiCol_ButtonActive, ImVec4(themeColor.x * 0.7f, themeColor.y * 0.7f, themeColor.z * 0.7f, 1.0f)); // Changed this line
                } else {
                    // Dimmed when disabled (roughly 40-50% of original brightness)
                    ImGui::PushStyleColor(ImGuiCol_Button, ImVec4(themeColor.x * 0.44f, themeColor.y * 0.7f, themeColor.z * 0.7f, 1.0f));
                    ImGui::PushStyleColor(ImGuiCol_ButtonHovered, ImVec4(themeColor.x * 0.56f, themeColor.y * 1.0f, themeColor.z * 1.0f, 1.0f));
                    ImGui::PushStyleColor(ImGuiCol_ButtonActive, ImVec4(themeColor.x * 0.375f, themeColor.y * 0.5f, themeColor.z * 0.5f, 1.0f));
                }

                if (ImGui::Button(label, ImVec2(-1, 20))) {
                    // Toggle panel visibility
                    if (current_option == label) {
                        current_option = "";
                    } else {
                        current_option = label;
                        currentImportedOption = "";
                    }
                }

                ImGui::PopStyleColor(3); // restore colors

                // Right-click toggle
                if (ImGui::IsItemHovered() && ImGui::IsMouseClicked(ImGuiMouseButton_Right)) {
                    enabled[GetIDFromCodeName(CodeName)] = !enabled[GetIDFromCodeName(CodeName)];
                }

                if (current_option == label) {
                    // Calculate available width for the child frame
                    float availableWidth = ImGui::GetContentRegionAvail().x;

                    if (CodeName == "Spam-Key") {
                        ImGui::BeginChild(std::string(label + std::string("_frame")).c_str(),
                                        ImVec2(availableWidth, 120), true);
                    } else {
                        ImGui::BeginChild(std::string(label + std::string("_frame")).c_str(),
                                        ImVec2(availableWidth, 80), true);
                    }

                    ImGui::TextWrapped("Enabled: %s", enabled[GetIDFromCodeName(CodeName)] ? "Yes" : "No");
                    ImGui::Separator();
                    ImGui::Text("Current keybind %s", input.getKeyName(Binds[CodeName]).c_str());
                    if (ImGui::Button("Change", ImVec2(-1, 20.0f))) {
                        bindToMacro(CodeName);
                    }
                    if (CodeName == "Spam-Key") {
                        ImGui::Text("Current spam key %s", input.getKeyName(SpamKey).c_str());
                        if (ImGui::Button("Bind spam key", ImVec2(-1, 20.0f))) {
                            BindSpamKey();
                        }
                    }
                    ImGui::EndChild();
                }
            };

            // Draw buttons
            DrawOptionButton("Freeze");
            DrawOptionButton("Extended Laugh Clip");
            DrawOptionButton("Extended Dance Clip");
            DrawOptionButton("Buckey clip");
            DrawOptionButton("Speed glitch");
            DrawOptionButton("Spam key");
            DrawOptionButton("Disable head collision");
            DrawOptionButton("NHC Roof Clip");
            DrawOptionButton("Helicopter High Jump");
            DrawOptionButton("Full Gear Desync");
            DrawOptionButton("Floor Bounce High Jump");
            DrawOptionButton("Wallhop");
            DrawOptionButton("Wallwalk");
            DrawOptionButton("FPS Dropper");

            // Draw all imported scripts
            DrawAllImportedScripts(themeColor);

            // Push custom colors for this button
            ImGui::PushStyleColor(ImGuiCol_Button, ImVec4(0.2f, 0.6f, 0.9f, 1.0f));
            ImGui::PushStyleColor(ImGuiCol_ButtonHovered, ImVec4(0.3f, 0.7f, 1.0f, 1.0f));
            ImGui::PushStyleColor(ImGuiCol_ButtonActive, ImVec4(0.1f, 0.5f, 0.8f, 1.0f));

            if (ImGui::Button("+ Import script", ImVec2(-1, 20))) {
                ImGui::SetNextWindowSize(ImGui::GetIO().DisplaySize);
                ImGui::SetNextWindowPos(ImVec2(0, 0));
                IGFD::FileDialogConfig config;
                config.path              = ".";
                config.flags             = 0;
                config.countSelectionMax = 1;

                ImGuiFileDialog::Instance()->OpenDialog(
                    "ChooseHSSfile",              // unique dialog key
                    "Import a script. -- Only get them from trusted sources!",                 // visible window title
                    ".hss",
                    config                           // config struct
                );
            }

            // Pop the colors back to normal
            ImGui::PopStyleColor(3);

            if (ImGuiFileDialog::Instance()->Display("ChooseHSSfile")) {
                if (ImGuiFileDialog::Instance()->IsOk()) {
                    std::string selectedFile = ImGuiFileDialog::Instance()->GetFilePathName();
                    if (importScript(selectedFile)) {
                        std::cout << "[UI] Successfully imported: " << selectedFile << std::endl;
                    }
                }
                ImGuiFileDialog::Instance()->Close();
            }



            ImGui::EndChild();

            // RIGHT PANEL: Use 0,0 to auto-fill remaining space
            ImGui::SameLine();
            ImGui::BeginChild("Right Panel", ImVec2(0, 0), true);
            if (current_option == "Freeze") {
                ImGui::Text("Freeze information:");
                ImGui::Separator();
                ImGui::TextWrapped("This macro freezes the roblox/sober process.\nIt allows for some pretty cool glitches.\n\nHere's a list of some glitches you can use it with.\n*All types of Lag high jumps\n*Extended dance/laugh clips\n*Cheer head glide\netc..");
            } else if (current_option == "Extended Laugh Clip") {
                ImGui::Text("Extended Laugh Clip information:");
                ImGui::Separator();
                ImGui::TextWrapped("This macro allows you to clip through walls of 1+ studs of thickness.\n\nTo use this macro, please set up yourself and the camera like below:\n");
                float windowWidth = ImGui::GetContentRegionAvail().x;
                float imageWidth = 248.0f;
                float offset = (windowWidth - imageWidth) * 0.5f;

                if (offset > 0.0f)
                    ImGui::SetCursorPosX(ImGui::GetCursorPosX() + offset);

                rlImGuiImageSize(&LoadedTextures[1], 248, 140);
                ImGui::TextWrapped("After this, it's pretty straightforward, just trigger the macro.\nJust remember, higher fps is better!\n");
            } else if (current_option == "Extended Dance Clip") {
                ImGui::Text("Extended Dance Clip information:");
                ImGui::Separator();
                ImGui::TextWrapped("This macro allows you to clip through walls of 1+ studs of thickness.\n\nTo use this macro, please set up yourself and the camera like below:\n");
                float windowWidth = ImGui::GetContentRegionAvail().x;
                float imageWidth = 248.0f;
                float offset = (windowWidth - imageWidth) * 0.5f;

                if (offset > 0.0f)
                    ImGui::SetCursorPosX(ImGui::GetCursorPosX() + offset);

                rlImGuiImageSize(&LoadedTextures[0], 248, 140);
                ImGui::TextWrapped("After this, it's pretty straightforward, just trigger the macro.\nYou might not get it the first attempt, try messing with the distance between you and the wall.\n");
            } else if (current_option == "Buckey clip") {
                ImGui::Text("Buckey Clip information:");
                ImGui::Separator();
                ImGui::TextWrapped("This macro allows you to clip through walls of 4+ studs of thickness.\n\nTo use this macro, please set up yourself and the camera like below:\n");
                float windowWidth = ImGui::GetContentRegionAvail().x;
                float imageWidth = 248.0f;
                float offset = (windowWidth - imageWidth) * 0.5f;

                if (offset > 0.0f)
                    ImGui::SetCursorPosX(ImGui::GetCursorPosX() + offset);

                rlImGuiImageSize(&LoadedTextures[2], 248, 140);
                ImGui::TextWrapped("After this, it's pretty straightforward, just trigger the macro! And you should clip through.");
            } else if (current_option == "Speed glitch") {
                ImGui::Text("Speed glitch information:");
                ImGui::Separator();
                ImGui::TextWrapped("This macro allows you to travel EXTREMELY fast! (800+ studs/s)");
                ImGui::TextWrapped("I won't go too much into details, look at this video to figure out how it works:");
                ImGui::PushItemWidth(ImGui::GetContentRegionAvail().x); // Fill horizontally

                static char url[] = "https://www.youtube.com/watch?v=5rmeivUegHc";
                ImGui::InputText("##url", url, sizeof(url), ImGuiInputTextFlags_ReadOnly);

                ImGui::PopItemWidth();

                ImGui::Spacing();
                ImGui::TextColored(orange,"Also, if you're willing to use this,\nPLEASE look in the 'Roblox specific'\nsection in settings.");
            } else if (current_option == "Spam key") {
                ImGui::Text("Spam key information:");
                ImGui::Separator();
                ImGui::TextWrapped("This macro allows you to spam a key in your keyboard very fast. You can use it to gear clip for example.\n\nTo gear clip, please set up yourself and the camera like below:\n");
                float windowWidth = ImGui::GetContentRegionAvail().x;
                float imageWidth = 248.0f;
                float offset = (windowWidth - imageWidth) * 0.5f;

                if (offset > 0.0f)
                    ImGui::SetCursorPosX(ImGui::GetCursorPosX() + offset);

                rlImGuiImageSize(&LoadedTextures[3], 248, 140);
                ImGui::TextWrapped("After this, it's pretty straightforward, just trigger the macro and hold W! This glitch can be rng, but you'll get it, especially with low fps.");
            } else if (current_option == "Disable head collision") {
                ImGui::Text("Disable head collision information:");
                ImGui::Separator();
                ImGui::TextWrapped("This macro allows you to disable your head collision with laugh clipping, this is necessary for some other fun glitches\n\nTo do this, please set up yourself and the camera as close as below:\n");
                float windowWidth = ImGui::GetContentRegionAvail().x;
                float imageWidth = 248.0f;
                float offset = (windowWidth - imageWidth) * 0.5f;

                if (offset > 0.0f)
                    ImGui::SetCursorPosX(ImGui::GetCursorPosX() + offset);

                rlImGuiImageSize(&LoadedTextures[4], 248, 140);
                ImGui::TextWrapped("After this, it's pretty straightforward, just trigger the macro! This glitch can be rng, but you'll get it.");
            } else if (current_option == "NHC Roof Clip") {
                ImGui::Text("NHC Roof Clip information:");
                ImGui::Separator();
                ImGui::TextColored(orange, "Targetted FPS: 40-60.\nNeeds no head collision.");
                if (ImGui::Button("Disable head collision")) {
                    current_option = "Disable head collision";
                }
                ImGui::TextWrapped("This macro allows you to clip through a 1+ stud thick roof with no head collision.\n\nTo do this, please have make sure you have a platform above your head like this.\n");
                float windowWidth = ImGui::GetContentRegionAvail().x;
                float imageWidth = 248.0f;
                float offset = (windowWidth - imageWidth) * 0.5f;

                if (offset > 0.0f)
                    ImGui::SetCursorPosX(ImGui::GetCursorPosX() + offset);

                rlImGuiImageSize(&LoadedTextures[5], imageWidth, 140);
                ImGui::TextWrapped("After this, it's pretty straightforward, just trigger the macro! This glitch can be rng, but you'll get it.\nAlso make sure you have head collision disabled!!!");
            } else if (current_option == "Helicopter High Jump") {
                ImGui::Text("Helicopter High Jump information:");
                ImGui::Separator();
                ImGui::TextWrapped("This macro allows you to jump VERY high if used proprely (20+ STUDS)");
                ImGui::TextWrapped("I won't go too much into details, look at this video to figure out how it works:");
                ImGui::PushItemWidth(ImGui::GetContentRegionAvail().x); // Fill horizontally

                static char url[] = "https://www.youtube.com/watch?v=5rmeivUegHc";
                ImGui::InputText("##url", url, sizeof(url), ImGuiInputTextFlags_ReadOnly);

                ImGui::PopItemWidth();

                ImGui::Spacing();
                ImGui::TextColored(orange,"Also, if you're willing to use this,\nPLEASE look in the 'Roblox specific'\nsection in settings.");
            } else if (current_option == "Full Gear Desync") {
                ImGui::Text("Full Gear Desync information:");
                ImGui::Separator();
                ImGui::TextWrapped("This macro allows you desynchronize an item from you (the client) and the server, allowing for some pretty cool glitches.");
                ImGui::TextWrapped("To do this, please have make sure you have 2 gears in item slot 1 and 2 UNEQUIPPED like below.\n");
                float windowWidth = ImGui::GetContentRegionAvail().x;
                float imageWidth = 110.0f;
                float offset = (windowWidth - imageWidth) * 0.5f;

                if (offset > 0.0f)
                    ImGui::SetCursorPosX(ImGui::GetCursorPosX() + offset);

                rlImGuiImageSize(&LoadedTextures[6], imageWidth, 58);
                ImGui::TextWrapped("Look at this video for more details on what you can do with this glitch");
                ImGui::PushItemWidth(ImGui::GetContentRegionAvail().x); // Fill horizontally

                static char url[] = "https://www.youtube.com/watch?v=ntK8Yus2odM";
                ImGui::InputText("##url", url, sizeof(url), ImGuiInputTextFlags_ReadOnly);

                ImGui::PopItemWidth();

                ImGui::Spacing();
            }  else if (current_option == "Floor Bounce High Jump") {
                ImGui::Text("Floor Bounce High Jump information:");
                ImGui::Separator();
                ImGui::TextWrapped("This macro allows you to jump 15+ studs vertically without anything!");
                ImGui::TextWrapped("To do this, just. Have 240 fps or more. That's it.\n\n");
                ImGui::TextWrapped("After this, it's pretty straightforward, just trigger the macro! And you should get the high jump once every 3 attempts.");
            } else if (current_option == "Wallhop") {
                ImGui::Text("Wallhop information:");
                ImGui::Separator();
                ImGui::TextColored(orange, "Targetted FPS: 60.\n");
                ImGui::TextWrapped("This macro allows you to 'double jump' if there are 2 parts one above another in a wall.\n\nTo do this, please have make sure you are set up like this.\n");
                float windowWidth = ImGui::GetContentRegionAvail().x;
                float imageWidth = 248.0f;
                float offset = (windowWidth - imageWidth) * 0.5f;

                if (offset > 0.0f)
                    ImGui::SetCursorPosX(ImGui::GetCursorPosX() + offset);

                rlImGuiImageSize(&LoadedTextures[7], imageWidth, 140);
                ImGui::TextWrapped("After this, it's pretty straightforward, just trigger the macro! You can also do it horizontally. But if you can do that then you probably don't need this stupid macro.");
            } else if (current_option == "Wallwalk") {
                ImGui::Text("Wallwalk information:");
                ImGui::Separator();
                ImGui::TextColored(orange, "Targetted FPS: 30-60.\n");
                ImGui::TextWrapped("This macro allows you to walk inside walls if there are 2 parts one above another. \n\nTo do this, please have make sure you are set up like this.\n");
                float windowWidth = ImGui::GetContentRegionAvail().x;
                float imageWidth = 248.0f;
                float offset = (windowWidth - imageWidth) * 0.5f;

                if (offset > 0.0f)
                    ImGui::SetCursorPosX(ImGui::GetCursorPosX() + offset);

                rlImGuiImageSize(&LoadedTextures[8], imageWidth, 140);
                ImGui::TextWrapped("After this, it's pretty straightforward, just trigger the macro, holding down W and D.");
            } else if (current_option == "FPS Dropper") {
                ImGui::Text("FPS Dropper information:");
                ImGui::Separator();
                ImGui::TextColored(orange, "Targetted FPS: 60.");
                ImGui::TextWrapped("This macro allows you to throttle your roblox FPS. The way it works is by freezing the process repeatedly. Just make sure you're on 60 FPS before trying to use it.");
                ImGui::TextWrapped("It isn't guaranteed for this to set your FPS cap as you like, but it's as consistent as it possibly can get.");

                ImGui::Spacing();
                ImGui::Text("Target FPS:");
                ImGui::SameLine();
                ImGui::SetNextItemWidth(80);
                ImGui::PushStyleVar(ImGuiStyleVar_FramePadding, ImVec2(4, 2));
                if (ImGui::InputText("##target_fps", FpsDrop::fps_input, sizeof(FpsDrop::fps_input), ImGuiInputTextFlags_CharsDecimal)) {
                    unsigned long val = strtoul(FpsDrop::fps_input, nullptr, 10);
                    if (val > 0) {
                        FpsDrop::target_fps = static_cast<unsigned int>(val);
                    }
                }
                ImGui::PopStyleVar();
                ImGui::SameLine();
                ImGui::TextDisabled("(Current: %u)", FpsDrop::target_fps);
            } else if (!currentImportedOption.empty()) {
                DrawImportedScriptRightPanel();
            } else {
                // Window padding and style
                ImGui::PushStyleVar(ImGuiStyleVar_WindowPadding, ImVec2(15, 15));
                ImGui::PushStyleColor(ImGuiCol_Header, ImVec4(0.2f, 0.2f, 0.25f, 1.0f));

                // Greeting box
                ImGui::BeginChild("Greeting", ImVec2(0, 60), true);
                std::string greeting = logzz::current_username.empty()
                    ? "Hello owner!"
                    : "Hello " + logzz::current_username + "!";
                ImGui::TextWrapped("%s", greeting.c_str());
                ImGui::TextWrapped("Welcome to Roblox Hypersuite!");
                ImGui::EndChild();

                ImGui::Spacing();
                ImGui::Separator();
                ImGui::Spacing();

                // Info section with bullets
                ImGui::TextWrapped("↓ Here are some things to know:");
                ImGui::Spacing();

                ImGui::Bullet(); ImGui::TextColored(orange, "Right click to toggle macro");
                ImGui::Bullet(); ImGui::TextColored(orange, "Left click to view macro info");
                ImGui::Bullet(); ImGui::TextColored(orange, "Hold left click and drag to\nmove the window");

                ImGui::Spacing();
                ImGui::TextWrapped("Check out the Settings tab! It might be useful to you.");

                // Restore styles
                ImGui::PopStyleColor();
                ImGui::PopStyleVar();
            }
            ImGui::EndChild();

            ImGui::EndTabItem();
        }

        // Lag switch
        if (ImGui::BeginTabItem("Lag-switch")) {
            ImGui::Text("Lag switch settings:");
            ImGui::Separator();
            ImGui::Checkbox("Enabled Keybind", &enabled[4]);
            ImGui::Text("Current keybind %s", input.getKeyName(Binds["Lag-switch"]).c_str());

            if (LagSwitchNamespace::TrafficBlocked == true) {
                ImGui::SameLine();
                ImGui::TextColored(ImVec4(1.0f,0.5f,0,1.0f), "                     Lag-switch currently enabled!");
            }

            if (ImGui::Button("Change", ImVec2(80.0f, 20.0f))) {
                bindToMacro("Lag-switch");
            }

            ImGui::Checkbox("Prevent disconnection", &LagSwitchNamespace::PreventDisconnection);
            ImGui::Checkbox("Allow advanced settings", &LagSwitchNamespace::customValuesAllowed);
            if (LagSwitchNamespace::customValuesAllowed) {
                ImGui::SliderFloat("Packet loss chance %", &LagSwitchNamespace::PacketLossPercentage, 0.0f, 100.0f);
                ImGui::InputInt("Ping increase (ms)", &LagSwitchNamespace::LagTimeMilliseconds);
            }

            ImGui::EndTabItem();
        }
        renderRobloxSettingsWindow();// global settings

        if (ImGui::BeginTabItem("Server hop")) {
            ImGui::Text("Hypersuite server hopper:");
            ImGui::Separator();

            // state variables
            static bool wasRunningBeforeRestart = false;
            static bool isRestarting = false;
            static bool showRestartSuccess = false;
            static float restartMessageTimer = 0.0f;
            static std::string restartStatusMsg = "";
            static bool showPlaceFetchMsg = false;
            static bool placeFetchSuccess = false;
            static float placeFetchTimer = 0.0f;
            static bool showInstanceFetchMsg = false;
            static bool instanceFetchSuccess = false;
            static float instanceFetchTimer = 0.0f;
            static bool showHopMsg = false;
            static bool hopSuccess = false;
            static float hopMsgTimer = 0.0f;
            static std::string hopStatusMsg = "";

            // === PLACE ID SECTION ===
            ImGui::AlignTextToFramePadding();
            ImGui::Text("Place ID:");
            ImGui::SameLine(80);
            ImGui::SetNextItemWidth(130);
            ImGui::InputText("##PlaceID", placeIdBuffer, sizeof(placeIdBuffer));
            ImGui::SameLine();
            if (ImGui::Button("Fetch##PlaceID", ImVec2(60, 0))) {
                unsigned long long lastPlaceID = getLastPlaceID();
                if (lastPlaceID != 0) {
                    std::string placeIDStr = std::to_string(lastPlaceID);
                    std::snprintf(placeIdBuffer, sizeof(placeIdBuffer), "%s", placeIDStr.c_str());
                    placeFetchSuccess = true;
                } else {
                    placeFetchSuccess = false;
                }
                showPlaceFetchMsg = true;
                placeFetchTimer = 2.0f;
            }

            if (showPlaceFetchMsg && placeFetchTimer > 0.0f) {
                ImGui::SameLine();
                ImGui::TextColored(placeFetchSuccess ? ImVec4(0.3f, 1.0f, 0.3f, 1.0f) : ImVec4(1.0f, 0.3f, 0.3f, 1.0f),
                                  placeFetchSuccess ? "OK" : "Fail");
                placeFetchTimer -= ImGui::GetIO().DeltaTime;
                if (placeFetchTimer <= 0.0f) showPlaceFetchMsg = false;
            }

            // === INSTANCE ID SECTION ===
            ImGui::AlignTextToFramePadding();
            ImGui::Text("Instance:");
            ImGui::SameLine(80);
            ImGui::SetNextItemWidth(130);

            bool hasPlaceId = strlen(placeIdBuffer) > 0;
            if (!hasPlaceId) ImGui::BeginDisabled();

            ImGui::InputText("##InstanceID", instanceIdBuffer, sizeof(instanceIdBuffer));
            ImGui::SameLine();
            if (ImGui::Button("Fetch##InstanceID", ImVec2(60, 0))) {
                std::string lastInstanceID = getLastInstanceID();
                if (!lastInstanceID.empty()) {
                    std::snprintf(instanceIdBuffer, sizeof(instanceIdBuffer), "%s", lastInstanceID.c_str());
                    instanceFetchSuccess = true;
                } else {
                    instanceFetchSuccess = false;
                }
                showInstanceFetchMsg = true;
                instanceFetchTimer = 2.0f;
            }

            if (!hasPlaceId) ImGui::EndDisabled();

            if (showInstanceFetchMsg && instanceFetchTimer > 0.0f) {
                ImGui::SameLine();
                ImGui::TextColored(instanceFetchSuccess ? ImVec4(0.3f, 1.0f, 0.3f, 1.0f) : ImVec4(1.0f, 0.3f, 0.3f, 1.0f),
                                  instanceFetchSuccess ? "OK" : "Fail");
                instanceFetchTimer -= ImGui::GetIO().DeltaTime;
                if (instanceFetchTimer <= 0.0f) showInstanceFetchMsg = false;
            }

            ImGui::Spacing();

            // === MIN FREE SLOTS ===
            ImGui::AlignTextToFramePadding();
            ImGui::Text("Min slots:");
            ImGui::SameLine(80);
            ImGui::SetNextItemWidth(60);
            ImGui::SliderInt("##MinSlots", &ServerHopper::minFreeSlots, 1, 10);

            ImGui::Spacing();
            ImGui::Separator();
            ImGui::Spacing();

            // === SERVER HOP BUTTON ===
            if (!hasPlaceId) ImGui::BeginDisabled();

            if (ImGui::Button("Server Hop", ImVec2(90, 0))) {
                std::string newServerID;
                hopSuccess = ServerHopper::HopToNewServer(placeIdBuffer, newServerID);

                if (hopSuccess) {
                    // Set instance ID and restart
                    std::snprintf(instanceIdBuffer, sizeof(instanceIdBuffer), "%s", newServerID.c_str());
                    hopStatusMsg = "Hopping...";

                    // Restart Roblox with new server
                    wasRunningBeforeRestart = (logzz::current_state == IN_LUA_APP || logzz::current_state == IN_GAME);
                    restartRoblox();
                } else {
                    hopStatusMsg = ServerHopper::lastError;
                }

                showHopMsg = true;
                hopMsgTimer = 3.0f;
            }

            if (!hasPlaceId) ImGui::EndDisabled();

            if (showHopMsg && hopMsgTimer > 0.0f) {
                ImGui::SameLine();
                ImGui::TextColored(hopSuccess ? ImVec4(0.3f, 1.0f, 0.3f, 1.0f) : ImVec4(1.0f, 0.3f, 0.3f, 1.0f),
                                  "%s", hopStatusMsg.c_str());
                hopMsgTimer -= ImGui::GetIO().DeltaTime;
                if (hopMsgTimer <= 0.0f) showHopMsg = false;
            }

            ImGui::SameLine();
            if (ImGui::Button("Clear List", ImVec2(80, 0))) {
                ServerHopper::ClearBannedServers();
            }

            ImGui::Spacing();
            ImGui::Separator();
            ImGui::Spacing();

            // === BANNED SERVERS LIST ===
            ImGui::Text("Visited servers: %zu", ServerHopper::bannedServerIDs.size());

            ImGui::BeginChild("##BannedServers", ImVec2(0, 120), true);

            if (ServerHopper::bannedServerIDs.empty()) {
                ImGui::TextDisabled("No servers visited yet");
            } else {
                static int toRemove = -1;
                for (size_t i = 0; i < ServerHopper::bannedServerIDs.size(); i++) {
                    ImGui::PushID(i);

                    // Show server ID
                    ImGui::Text("%s", ServerHopper::bannedServerIDs[i].c_str());

                    // Remove button
                    ImGui::SameLine(ImGui::GetContentRegionAvail().x - 50);
                    if (ImGui::SmallButton("Remove")) {
                        toRemove = i;
                    }

                    ImGui::PopID();
                }

                // Remove outside the loop
                if (toRemove >= 0) {
                    ServerHopper::bannedServerIDs.erase(ServerHopper::bannedServerIDs.begin() + toRemove);
                    toRemove = -1;
                }
            }

            ImGui::EndChild();

            ImGui::Spacing();
            ImGui::Separator();
            ImGui::Spacing();

            // === ROBLOX CONTROL ===
            ImGui::Text("Roblox control:");
            ImGui::Spacing();

            if (ImGui::Button("Restart / Start Roblox", ImVec2(160, 0))) {
                if (logzz::current_state == IN_LUA_APP || logzz::current_state == IN_GAME) {
                    wasRunningBeforeRestart = true;
                } else {
                    wasRunningBeforeRestart = false;
                }
                isRestarting = true;
                restartStatusMsg = wasRunningBeforeRestart ? "Restarting..." : "Starting...";
                restartMessageTimer = 0.0f;
                showRestartSuccess = true;
                restartRoblox();
            }

            if (isRestarting) {
                restartMessageTimer += ImGui::GetIO().DeltaTime;
                if (restartMessageTimer > 2.0f) {
                    if (logzz::current_state == IN_LUA_APP || logzz::current_state == IN_GAME) {
                        restartStatusMsg = wasRunningBeforeRestart ? "Restarted!" : "Started!";
                        isRestarting = false;
                        restartMessageTimer = 2.0f;
                    }
                }
            }

            if (showRestartSuccess && restartMessageTimer > 0.0f) {
                ImGui::SameLine();
                ImGui::TextColored(isRestarting ? ImVec4(1.0f, 0.8f, 0.2f, 1.0f) : ImVec4(0.3f, 1.0f, 0.3f, 1.0f),
                                  "%s", restartStatusMsg.c_str());
                if (!isRestarting) {
                    restartMessageTimer -= ImGui::GetIO().DeltaTime;
                    if (restartMessageTimer <= 0.0f) showRestartSuccess = false;
                }
            }

            ImGui::EndTabItem();
        }

        if (ImGui::BeginTabItem("Hypersuite")) {
            // ==== GLOBAL SETTINGS ====
            ImGui::Text("Global settings:");
            ImGui::Separator();

            // Keyboard layout
            ImGui::Text("Keyboard layout:");
            ImGui::PushItemWidth(100);
            if (ImGui::Combo("##kb_layout", (int*)&kb_layout, string_kb_layouts, IM_ARRAYSIZE(string_kb_layouts))) {
                printf("Selected layout: %s (index %d)\n", string_kb_layouts[kb_layout], kb_layout);
            }
            ImGui::SameLine();
            if (ImGui::Button("Bind chat key")) {
                BindVariable(&ChatKey);
            }
            ImGui::SameLine();
            //ImGui::Text(("Current: " + std::string(input.getKeyName(ChatKey))).c_str()); -- Prevents building on nixos for some reason.
            ImGui::Text("%s", ("Current: " + std::string(input.getKeyName(ChatKey))).c_str());
            ImGui::Checkbox("Window always on top", &windowOnTop);
#ifdef _WIN32
            ImGui::Checkbox("Decorated window (title bar) (100% DPI recommended)", &decorated_window);
#endif
            ImGui::PopItemWidth();

            ImGui::Spacing();
            ImGui::Separator();

            // ==== ROBLOX SETTINGS ====
            ImGui::Text("Roblox specific:");

            // Start 2 columns
            ImGui::Columns(2, nullptr, false);

            // ---------------- LEFT COLUMN ----------------
            ImGui::PushItemWidth(100);

            // FPS
            ImGui::Text("FPS:");
            ImGui::SameLine(120); // align value
            if (ImGui::InputInt("##fps", &roblox_fps)) {
                updateSpeedglitchFPS(roblox_fps);
            }

            // Executable Name
            ImGui::Text("Process:");
            ImGui::SameLine(120);

            static char process_name_buffer[256] = "";
            static bool buffer_initialized = false;

            if (!buffer_initialized) {
                strncpy(process_name_buffer, roblox_process_name.c_str(), sizeof(process_name_buffer) - 1);
                buffer_initialized = true;
            }

            if (ImGui::InputText("##exec", process_name_buffer, sizeof(process_name_buffer))) {
                roblox_process_name = process_name_buffer;
            }

            ImGui::PopItemWidth();

            ImGui::NextColumn();

            // ---------------- RIGHT COLUMN ----------------

            // Checkbox
            ImGui::Text("Cam-Fix:");
            ImGui::SameLine(120);
            if (ImGui::Checkbox("##camfix", &cam_fix_active)) {
                updateSpeedglitchSensitivity(roblox_sensitivity, cam_fix_active);
                calculateWallhopPixels();
                calculateWallwalkPixels();
            }

            ImGui::PushItemWidth(100);

            // Sensitivity
            ImGui::Text("Sensitivity:");
            ImGui::SameLine(120);
            if (ImGui::InputFloat("##sens", &roblox_sensitivity, 0.1f, 0.5f, "%.2f")) {
                if (roblox_sensitivity < 0.1f) roblox_sensitivity = 0.1f;
                if (roblox_sensitivity > 4.0f) roblox_sensitivity = 4.0f;
                updateSpeedglitchSensitivity(roblox_sensitivity, cam_fix_active);
                calculateWallhopPixels();
                calculateWallwalkPixels();
            }

            ImGui::PopItemWidth();

            ImGui::Columns(1); // end columns
            ImGui::Spacing();
            ImGui::Separator();

            // ==== THEME ====
            ImGui::Text("Theme color:");
            if (ImGui::ColorEdit3("##theme_color", (float*)&themeColor)) {
                applyThemeColor(themeColor);
            }

            ImGui::Text("Presets:");

            auto ColorPresetButton = [&](const char* id, const ImVec4& color) {
                ImGui::PushStyleColor(ImGuiCol_Button, color);
                ImGui::PushStyleColor(ImGuiCol_ButtonHovered, ImVec4(color.x * 1.2f, color.y * 1.2f, color.z * 1.2f, 1.0f));
                ImGui::PushStyleColor(ImGuiCol_ButtonActive, ImVec4(color.x * 0.8f, color.y * 0.8f, color.z * 0.8f, 1.0f));
                bool clicked = ImGui::Button(id, ImVec2(20, 20));
                ImGui::PopStyleColor(3);
                return clicked;
            };

            const ImVec4 presetColors[] = {
                {0.6f, 0.08f, 0.08f, 1.0f},   // red
                {0.08f, 0.3f, 0.6f, 1.0f},    // blue
                {0.08f, 0.5f, 0.15f, 1.0f},   // green
                {0.45f, 0.15f, 0.6f, 1.0f},   // purple
                {0.7f, 0.35f, 0.08f, 1.0f},   // orange
                {0.08f, 0.5f, 0.55f, 1.0f},   // cyan
                {0.65f, 0.15f, 0.4f, 1.0f},   // pink
                {0.65f, 0.55f, 0.08f, 1.0f},   // yellow
                {0.08f, 0.45f, 0.4f, 1.0f},   // teal
                {0.55f, 0.08f, 0.55f, 1.0f},  // magenta
                {0.4f, 0.65f, 0.08f, 1.0f},   // lime
                {50/255.f, 50/255.f, 50/255.f, 1.0f}, // silver
            };

            for (int i = 0; i < IM_ARRAYSIZE(presetColors); i++) {
                char id[16];
                sprintf(id, "##c%d", i);
                if (ColorPresetButton(id, presetColors[i])) {
                    themeColor = presetColors[i];
                    applyThemeColor(themeColor);
                }
                if (i != IM_ARRAYSIZE(presetColors) - 1)
                    ImGui::SameLine();
            }

            ImGui::Spacing();

            ImGui::Checkbox("Rainbow Theme", &rainbowThemeEnabled);
            if (rainbowThemeEnabled) {
                ImGui::SameLine();
                ImGui::TextColored(ImVec4(1.f, 0.5f, 0.f, 1.f), "(cycling)");

                ImGui::Text("Rainbow Theme Settings:");
                ImGui::SliderFloat("Hue Speed", &rainbowSpeed, 0.001f, 0.02f, "%.3f");
                ImGui::SliderFloat("Saturation", &rainbowSaturation, 0.0f, 1.0f, "%.2f");
                ImGui::SliderFloat("Value", &rainbowValue, 0.0f, 1.0f, "%.2f");
            }

            ImGui::Separator();
            ImGui::Text("Made with love <3 -3443");

            ImGui::EndTabItem();
        }

        ImGui::EndTabBar();
    }

    ImGui::End();
}

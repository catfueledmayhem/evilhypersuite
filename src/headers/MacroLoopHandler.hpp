#pragma once
#include "Globals.hpp"
#include "Macros.hpp"
#include "LagSwitch.hpp"
#include "Speedglitch.hpp"
#include "HHJ.hpp"
#include "GearDesync.hpp"
#include "WallhopAndWallwalk.hpp"
#include "FPSDropper.hpp"
#include "hsscript.hpp"
#include "hsscriptman.hpp"
#include "ImportedScriptsUI.hpp"

inline void initMacros() {
    initSpeedglitch();
    initHHJ();
    initGearDesync();
    initWallhop();
    initWallwalk();
    FpsDrop::Init();

    // evil, very evil.............
    initScriptSystem();
}

inline void UpdateMacros() {
    if (is_elevated) {}
    if (enabled[0]) freezeMacro();
    if (enabled[1]) laughClip();
    if (enabled[2]) extendedDanceClip();
    if (enabled[4]) LagSwitch();
    if (enabled[5]) BuckeyClip();
    if (enabled[6]) speedglitchMacro();
    if (enabled[7]) SpamKeyMacro();
    if (enabled[10]) DisableHeadCollision();
    if (enabled[11]) NHCRoofClip();
    if (enabled[12]) helicopterHighJump();
    //if (enabled[13]) gearDesyncMacro();
    if (enabled[14]) FullGearDesync();
    if (enabled[15]) FloorBounceHighJump();
    if (enabled[16]) wallhopMacro();
    if (enabled[17]) wallwalkMacro();
    if (enabled[18]) FpsDrop::Macro();

    HandleImportedScriptKeybindCapture(input);
    updateImportedScripts(input);
}

inline void CleanupMacros() {
    cleanupWallhop();
    cleanupWallwalk();

    // we did it boys.
    cleanupAllScripts();
}

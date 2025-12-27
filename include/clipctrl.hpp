// clipctrl.hpp - Simple Windows clipboard library
#ifndef CLIPCTRL_HPP
#define CLIPCTRL_HPP

#ifdef _WIN32
    #define WIN32_LEAN_AND_MEAN
    #include <windows.h>
    #include <string.h>
#endif

static bool clipboard_set_text(const char* text) {
#ifdef _WIN32
    if (!text) return false;

    size_t len = strlen(text);
    HGLOBAL hMem = GlobalAlloc(GMEM_MOVEABLE, len + 1);
    if (!hMem) return false;

    memcpy(GlobalLock(hMem), text, len + 1);
    GlobalUnlock(hMem);

    if (!OpenClipboard(NULL)) {
        GlobalFree(hMem);
        return false;
    }

    EmptyClipboard();
    bool success = SetClipboardData(CF_TEXT, hMem) != NULL;
    CloseClipboard();

    if (!success) GlobalFree(hMem);
    return success;
#else
    return false;
#endif
}

#endif // CLIPCTRL_HPP

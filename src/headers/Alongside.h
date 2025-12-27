#pragma once
#include <string>

//-- Required files to be next to the program.
const char *files_req[] = {
#ifdef _WIN32
    "WinDivert.dll",
    "WinDivert64.sys",
#endif
    NULL
};

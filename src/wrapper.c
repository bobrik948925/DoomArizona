#include <windows.h>
#include <string.h>
#include "doomgeneric.h"

static char g_wadPath[1024];

static DWORD WINAPI DoomThread(LPVOID param) {
    char *argv[3];
    argv[0] = "doom";
    argv[1] = "-iwad";
    argv[2] = g_wadPath;
    doomgeneric_Create(3, argv);
    for (;;) {
        doomgeneric_Tick();
    }
    return 0;
}

__declspec(dllexport) int StartDoom(const char *wadPath) {
    if (wadPath) {
        strncpy(g_wadPath, wadPath, sizeof(g_wadPath)-1);
        g_wadPath[sizeof(g_wadPath)-1] = 0;
    } else {
        g_wadPath[0] = 0;
    }
    CreateThread(NULL, 0, DoomThread, NULL, 0, NULL);
    return 0;
}

/* DllMain provided by doomgeneric_win.c */

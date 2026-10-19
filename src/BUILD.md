# Building doom.dll

This DLL is a 32-bit (x86) build of doomgeneric with a custom mimgui/D3D9 backend.

## Requirements
- TinyCC (32-bit) or MinGW (32-bit)
- Original doomgeneric source: https://github.com/ozkl/doomgeneric

## Steps
1. Download doomgeneric source and copy all files from doomgeneric/ into this src/ folder.
2. Keep our modified doomgeneric_win.c, wrapper.c, winnls.h.
3. Remove other backends: doomgeneric_allegro.c, doomgeneric_sdl.c, doomgeneric_xlib.c, doomgeneric_linuxvt.c, doomgeneric_soso.c, doomgeneric_sosox.c, doomgeneric_emscripten.c, and i_sdl*/i_allegro* files.
4. Create dvapi32.def in the compiler lib folder with:
   RegOpenKeyExA
   RegQueryValueExA
   RegCloseKey
5. Compile (TinyCC example):
   tcc -shared -o doom.dll *.c -DSNDSERV -D_DEFAULT_SOURCE -D_WIN32 -DCP_ACP=0 -DCP_OEMCP=1 -DCP_UTF8=65001 -include winnls.h -I. -luser32 -lgdi32 -lkernel32 -ladvapi32 -lmsvcrt
6. Ensure output is 32-bit (x86). Place doom.dll next to Doom.lua (moonloader\\doom\\).

## Notes
- The DLL exports: DoomInit, DoomGetTexture, DoomUpdateTexture.
- DoomInit(devicePtr, wadPath) creates a D3D9 texture and starts the Doom thread.
- Doom runs in a background thread; texture upload happens on the main thread via DoomUpdateTexture().

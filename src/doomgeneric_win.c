#include "doomkeys.h"
#include "doomgeneric.h"

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <windows.h>

/* Minimal D3D9 COM vtable wrappers */
typedef struct IDirect3DDevice9 IDirect3DDevice9;
typedef struct IDirect3DTexture9 IDirect3DTexture9;

typedef struct { int Pitch; void* pBits; } D3DLOCKED_RECT;

typedef struct IDirect3DDevice9Vtbl {
    void* QueryInterface;
    void* AddRef;
    void* Release;
    void* TestCooperativeLevel;
    void* GetAvailableTextureMem;
    void* EvictManagedResources;
    void* GetDirect3D;
    void* GetDeviceCaps;
    void* GetDisplayMode;
    void* GetCreationParameters;
    void* SetCursorProperties;
    void* SetCursorPosition;
    void* ShowCursor;
    void* CreateAdditionalSwapChain;
    void* GetSwapChain;
    void* GetNumberOfSwapChains;
    void* Reset;
    void* Present;
    void* GetBackBuffer;
    void* GetRasterStatus;
    void* SetDialogBoxMode;
    void* SetGammaRamp;
    void* GetGammaRamp;
    HRESULT (__stdcall *CreateTexture)(IDirect3DDevice9*, UINT, UINT, UINT, DWORD, DWORD, DWORD, IDirect3DTexture9**, HANDLE*);
    /* remaining not needed */
} IDirect3DDevice9Vtbl;

struct IDirect3DDevice9 { IDirect3DDevice9Vtbl* lpVtbl; };

typedef struct IDirect3DTexture9Vtbl {
    void* QueryInterface;
    void* AddRef;
    void* Release;
    void* GetDevice;
    void* SetPrivateData;
    void* GetPrivateData;
    void* FreePrivateData;
    void* SetPriority;
    void* GetPriority;
    void* PreLoad;
    void* GetType;
    void* SetLOD;
    void* GetLOD;
    void* GetLevelCount;
    void* SetAutoGenFilterType;
    void* GetAutoGenFilterType;
    void* GenerateMipSubLevels;
    void* GetLevelDesc;
    void* GetSurfaceLevel;
    HRESULT (__stdcall *LockRect)(IDirect3DTexture9*, UINT, D3DLOCKED_RECT*, const RECT*, DWORD);
    HRESULT (__stdcall *UnlockRect)(IDirect3DTexture9*, UINT);
    void* AddDirtyRect;
} IDirect3DTexture9Vtbl;

struct IDirect3DTexture9 { IDirect3DTexture9Vtbl* lpVtbl; };

#define D3DFMT_X8R8G8B8 22
#define D3DPOOL_MANAGED 1
#define D3DLOCK_DISCARD 0

static IDirect3DDevice9* g_device = NULL;
static IDirect3DTexture9* g_texture = NULL;
static int g_prevKeys[256];
static int g_hasPrev = 0;
static char g_wadPath[1024];

static CRITICAL_SECTION g_cs;
static int g_csInit = 0;
static pixel_t* g_frameBuffer = NULL;
static volatile int g_frameReady = 0;

static DWORD WINAPI DoomThreadProc(LPVOID param)
{
    (void)param;
    char* argv[3];
    argv[0] = "doom";
    argv[1] = "-iwad";
    argv[2] = g_wadPath;
    doomgeneric_Create(3, argv);
    for (;;) doomgeneric_Tick();
    return 0;
}

static unsigned char convertToDoomKey(unsigned char key)
{
    switch (key)
    {
    case VK_RETURN: key = KEY_ENTER; break;
    case VK_ESCAPE: key = KEY_ESCAPE; break;
    case VK_LEFT: key = KEY_LEFTARROW; break;
    case VK_RIGHT: key = KEY_RIGHTARROW; break;
    case VK_UP: key = KEY_UPARROW; break;
    case VK_DOWN: key = KEY_DOWNARROW; break;
    case VK_CONTROL: key = KEY_FIRE; break;
    case VK_SPACE: key = KEY_USE; break;
    case VK_SHIFT: key = KEY_RSHIFT; break;
    default: key = (unsigned char)tolower(key); break;
    }
    return key;
}

#define KEYQUEUE_SIZE 64
static unsigned short s_KeyQueue[KEYQUEUE_SIZE];
static unsigned int s_KeyQueueWriteIndex = 0;
static unsigned int s_KeyQueueReadIndex = 0;

static void addKeyToQueue(int pressed, unsigned char keyCode)
{
    unsigned char key = convertToDoomKey(keyCode);
    unsigned short keyData = (pressed << 8) | key;
    s_KeyQueue[s_KeyQueueWriteIndex] = keyData;
    s_KeyQueueWriteIndex = (s_KeyQueueWriteIndex + 1) % KEYQUEUE_SIZE;
}

int DG_GetKey(int* pressed, unsigned char* doomKey)
{
    if (s_KeyQueueReadIndex == s_KeyQueueWriteIndex) return 0;
    unsigned short keyData = s_KeyQueue[s_KeyQueueReadIndex];
    s_KeyQueueReadIndex = (s_KeyQueueReadIndex + 1) % KEYQUEUE_SIZE;
    *pressed = keyData >> 8;
    *doomKey = keyData & 0xFF;
    return 1;
}

void DG_Init()
{
    memset(s_KeyQueue, 0, sizeof(s_KeyQueue));
    memset(g_prevKeys, 0, sizeof(g_prevKeys));
    g_hasPrev = 0;
}

void DG_DrawFrame()
{
    /* Poll keyboard */
    for (int i = 0; i < 256; i++) {
        SHORT st = GetAsyncKeyState(i);
        int pressed = (st & 0x8000) ? 1 : 0;
        if (!g_hasPrev) {
            g_prevKeys[i] = pressed;
        } else if (pressed != g_prevKeys[i]) {
            addKeyToQueue(pressed, (unsigned char)i);
            g_prevKeys[i] = pressed;
        }
    }
    g_hasPrev = 1;

    /* Copy framebuffer to system buffer (D3D upload done on main thread) */
    if (g_frameBuffer && DG_ScreenBuffer) {
        if (!g_csInit) { InitializeCriticalSection(&g_cs); g_csInit = 1; }
        EnterCriticalSection(&g_cs);
        memcpy(g_frameBuffer, DG_ScreenBuffer, DOOMGENERIC_RESX * DOOMGENERIC_RESY * sizeof(pixel_t));
        g_frameReady = 1;
        LeaveCriticalSection(&g_cs);
    }

    /* Also process window messages (none) */
    MSG msg;
    while (PeekMessageA(&msg, 0, 0, 0, PM_REMOVE) > 0) {
        TranslateMessage(&msg);
        DispatchMessageA(&msg);
    }
}

void DG_SleepMs(uint32_t ms) { Sleep(ms); }
uint32_t DG_GetTicksMs() { return GetTickCount(); }
void DG_SetWindowTitle(const char* title) { (void)title; }

/* Exports */
__declspec(dllexport) int DoomInit(void* devicePtr, const char* wadPath)
{
    if (!g_csInit) { InitializeCriticalSection(&g_cs); g_csInit = 1; }
    g_device = (IDirect3DDevice9*)devicePtr;
    if (g_device && !g_texture) {
        g_device->lpVtbl->CreateTexture(g_device, DOOMGENERIC_RESX, DOOMGENERIC_RESY, 1, 0, D3DFMT_X8R8G8B8, D3DPOOL_MANAGED, &g_texture, NULL);
    }
    if (!g_frameBuffer) {
        g_frameBuffer = (pixel_t*)malloc(DOOMGENERIC_RESX * DOOMGENERIC_RESY * sizeof(pixel_t));
    }
    if (wadPath) {
        strncpy(g_wadPath, wadPath, sizeof(g_wadPath)-1);
        g_wadPath[sizeof(g_wadPath)-1] = 0;
    } else g_wadPath[0] = 0;
    CreateThread(NULL, 0, DoomThreadProc, NULL, 0, NULL);
    return 1;
}

__declspec(dllexport) void* DoomGetTexture() { return (void*)g_texture; }

__declspec(dllexport) void DoomUpdateTexture()
{
    if (!g_texture || !g_frameBuffer) return;
    if (!g_csInit) return;
    EnterCriticalSection(&g_cs);
    if (g_frameReady) {
        D3DLOCKED_RECT locked;
        if (g_texture->lpVtbl->LockRect(g_texture, 0, &locked, NULL, 0) == 0) {
            char* dst = (char*)locked.pBits;
            pixel_t* src = g_frameBuffer;
            for (int y = 0; y < DOOMGENERIC_RESY; y++) {
                memcpy(dst + y * locked.Pitch, src + y * DOOMGENERIC_RESX, DOOMGENERIC_RESX * sizeof(pixel_t));
            }
            g_texture->lpVtbl->UnlockRect(g_texture, 0);
        }
        g_frameReady = 0;
    }
    LeaveCriticalSection(&g_cs);
}

BOOL APIENTRY DllMain(HMODULE hModule, DWORD reason, LPVOID reserved) { return TRUE; }

int main(int argc, char** argv) { (void)argc; (void)argv; return 0; }

#include <windows.h>
#include <shlwapi.h>
#include <stdio.h>
#include "MinHook/include/MinHook.h"

#include "config.h"
#include "macros.h"
#include "scheme.h"
#include "hotkey.h"
#include "overlay.h"

#define DEBUG_CONSOLE

#ifdef DEBUG_CONSOLE
#define log(format, ...) wprintf(format, ##__VA_ARGS__)
#else
#define log(format, ...) /* Nothing. */
#endif

#define GLOBAL static

#define SCHEME_DEFAULT L"////////"
#define TARGET_WND_NAME L"光·遇"
#define WM_USER_EXIT (0x8000 + 1)

typedef BOOL (WINAPI *PeekMessage_t)(LPMSG, HWND, UINT, UINT, UINT);

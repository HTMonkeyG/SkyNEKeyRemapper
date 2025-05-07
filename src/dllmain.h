#include <windows.h>
#include <shlwapi.h>
#include <stdio.h>
#include "MinHook/include/MinHook.h"

#include "config.h"
#include "macros.h"
#include "scheme.h"
#include "hotkey.h"

#define SCHEME_DEFAULT L"////////"
#define WM_USER_EXIT (0x8000 + 1)

#define DEBUG_CONSOLE

typedef BOOL (WINAPI *PeekMessage_t)(LPMSG, HWND, UINT, UINT, UINT);

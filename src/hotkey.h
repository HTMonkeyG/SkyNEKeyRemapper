#include <windows.h>
#include <stdio.h>
#include "macros.h"

#define VK_LMASK 0x00FF
#define VK_HMASK 0xFF00
#define VK_MODSHIFT 0x0400
#define VK_MODCTRL 0x0200
#define VK_MODALT 0x0100
#define VK_MODWIN 0x0800

typedef struct {
  UINT mod;
  UINT vk;
} Hotkey_t;

i32 getKeyFromName(const wchar_t *name, const wchar_t **endp);
i32 buildHotkeyFrom(const wchar_t *desc, Hotkey_t *hk);
i32 registerHotkeyWith(HWND hWnd, i32 id, const Hotkey_t *hk);

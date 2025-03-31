#include "dllmain.h"

extern unsigned short normalKeys[256]
  , extendedKeys[256];

typedef BOOL (WINAPI *PeekMessage_t)(LPMSG, HWND, UINT, UINT, UINT);
PeekMessage_t PeekMessageW_old, PeekMessageA_old;

void modifyMsg(LPMSG lpMsg) {
  unsigned char scanCode, isExtended;
  unsigned short *config;
  if (!lpMsg)
    return;

  if (lpMsg->message == WM_KEYDOWN || lpMsg->message == WM_KEYUP) {
    scanCode = (lpMsg->lParam & 0x00FF0000) >> 16;
    isExtended = (lpMsg->lParam & 0x01000000) >> 24;
    config = isExtended ? extendedKeys : normalKeys;
    if (config[scanCode]) {
      lpMsg->lParam &= (~0x01FF0000);
      lpMsg->lParam |= (config[scanCode] & 0xFF) << 16;
      (config[scanCode] & 0xFF00) && (lpMsg->lParam |= 0x01000000);
    }
  }
}

BOOL PeekMessageW_Listener(LPMSG lpMsg, HWND hWnd, UINT wMsgFilterMin, UINT wMsgFilterMax, UINT wRemoveMsg) {
  BOOL result;
  result = PeekMessageW_old(lpMsg, hWnd, wMsgFilterMin, wMsgFilterMax, wRemoveMsg);
  modifyMsg(lpMsg);
  return result;
}

BOOL PeekMessageA_Listener(LPMSG lpMsg, HWND hWnd, UINT wMsgFilterMin, UINT wMsgFilterMax, UINT wRemoveMsg) {
  BOOL result;
  result = PeekMessageA_old(lpMsg, hWnd, wMsgFilterMin, wMsgFilterMax, wRemoveMsg);
  modifyMsg(lpMsg);
  return result;
}

BOOL APIENTRY DllMain(HMODULE hModule, DWORD ul_reason_for_call, LPVOID lpReserved) {
  char *dir, path[MAX_PATH];
  FILE *fd;

  if (ul_reason_for_call == DLL_PROCESS_ATTACH) {
    MH_Initialize();
    MH_CreateHookApi(L"user32.dll", "PeekMessageW", PeekMessageW_Listener, (void *)&PeekMessageW_old);
    MH_CreateHookApi(L"user32.dll", "PeekMessageA", PeekMessageA_Listener, (void *)&PeekMessageA_old);

    if (!GetModuleFileNameA(hModule, path, MAX_PATH))
      goto BEGIN;

    dir = strrchr(path, '\\');
    if (!dir)
      goto BEGIN;
    dir && (*dir = 0);
    strcat(path, "\\skykey-config.txt");
    fd = fopen(path, "r");
    if (!fd)
      goto BEGIN;
    buildConfigFrom(fd);
    fclose(fd);

BEGIN:
    MH_EnableHook(MH_ALL_HOOKS);
  } else if (ul_reason_for_call == DLL_PROCESS_DETACH) {
    MH_DisableHook(MH_ALL_HOOKS);
    MH_Uninitialize();
  }
  return TRUE;
}
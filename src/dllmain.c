#include "dllmain.h"

typedef BOOL (WINAPI *PeekMessage_t)(LPMSG, HWND, UINT, UINT, UINT);
PeekMessage_t PeekMessageW_old, PeekMessageA_old;

void modifyMsg(LPMSG lpMsg) {
  if (!lpMsg)
    return;
  if (lpMsg->message == WM_KEYDOWN || lpMsg->message == WM_KEYUP) {
    if (lpMsg->wParam == '\t') {
      lpMsg->lParam &= (~0xFF0000);
      lpMsg->lParam |= 0x2A0000;
    } else if ((lpMsg->lParam & 0xFF0000) == 0x2A0000) {
      lpMsg->lParam &= (~0xFF0000);
      lpMsg->lParam |= 0x0F0000;
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

VOID init() {
  HMODULE hDll = LoadLibraryA("user32.dll");
  MH_Initialize();
  MH_CreateHookApi(L"user32.dll", "PeekMessageW", PeekMessageW_Listener, &PeekMessageW_old);
  MH_CreateHookApi(L"user32.dll", "PeekMessageA", PeekMessageA_Listener, &PeekMessageA_old);
  MH_EnableHook(MH_ALL_HOOKS);
  /*initHook(&hook_PeekMessageW, (void *)GetProcAddress(hDll, "PeekMessageW"), PeekMessageW_Listener);
  installHook(&hook_PeekMessageW);
  initHook(&hook_PeekMessageA, (void *)GetProcAddress(hDll, "PeekMessageA"), PeekMessageA_Listener);
  installHook(&hook_PeekMessageA);*/
}

BOOL APIENTRY DllMain(HMODULE hModule, DWORD ul_reason_for_call, LPVOID lpReserved) {
  if (ul_reason_for_call == DLL_PROCESS_ATTACH)
    init();
  else if (ul_reason_for_call == DLL_PROCESS_DETACH) {
    MH_DisableHook(MH_ALL_HOOKS);
    MH_Uninitialize();
  }
  return TRUE;
}
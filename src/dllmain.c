#include "dllmain.h"

typedef BOOL (WINAPI *PeekMessage_t)(LPMSG, HWND, UINT, UINT, UINT);

extern unsigned short normalKeys[256]
  , extendedKeys[256];

PeekMessage_t PeekMessageW_old, PeekMessageA_old;
char hookEnabled;
DWORD hotkeyThreadId;
HANDLE hHotkeyThread;

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

DWORD WINAPI hotkeyThread(LPVOID lpParam) {
  MSG msg;
 
  if (!RegisterHotKey(NULL, 1, MOD_CONTROL, VK_F5)) {
    MessageBoxW(NULL, L"注册快捷键失败", L"Error", MB_ICONERROR);
    return 1;
  }

  while (GetMessageW(&msg, NULL, 0, 0)) {
    if (msg.message == WM_USER_EXIT)
      // Terminate thread.
      PostQuitMessage(0);
    else if (msg.message == WM_QUIT)
      // Exit message loop.
      break;
    else if (msg.message != WM_HOTKEY || GetForegroundWindow() != FindWindowW(NULL, L"光·遇"))
      // Ignore other messages.
      continue;
    else if (msg.message == WM_HOTKEY && msg.wParam == 1) {
      if (hookEnabled) {
        hookEnabled = 0;
        MH_DisableHook(MH_ALL_HOOKS);
        printf("Hook disabled\n");
      } else {
        hookEnabled = 1;
        printf("Hook enabled\n");
        MH_EnableHook(MH_ALL_HOOKS);
      }
    }
  }

  UnregisterHotKey(NULL, 1);
  return msg.wParam;
}

BOOL APIENTRY DllMain(HMODULE hModule, DWORD ul_reason_for_call, LPVOID lpReserved) {
  char *dir, path[MAX_PATH];
  FILE *fd;

  if (ul_reason_for_call == DLL_PROCESS_ATTACH) {
#ifdef DEBUG_CONSOLE
    FreeConsole();
    AllocConsole();
    freopen("CONOUT$","w+t",stdout);
    freopen("CONIN$","r+t",stdin);
    printf("DLL injected\n");
#endif

    MH_Initialize();
    MH_CreateHookApi(
      L"user32.dll",
      "PeekMessageW",
      PeekMessageW_Listener,
      (void *)&PeekMessageW_old
    );
    MH_CreateHookApi(
      L"user32.dll",
      "PeekMessageA",
      PeekMessageA_Listener,
      (void *)&PeekMessageA_old
    );

    if (!GetModuleFileNameA(hModule, path, MAX_PATH))
      goto BEGIN;

    dir = strrchr(path, '\\');
    if (!dir)
      goto BEGIN;
    *dir = 0;
    strcat(path, "\\skykey-config.txt");
    fd = fopen(path, "r");
    if (!fd)
      goto BEGIN;
    buildConfigFrom(fd);
    fclose(fd);

BEGIN:
    hHotkeyThread = CreateThread(NULL, 0, hotkeyThread, 0, 0, &hotkeyThreadId);
    printf("%lx\n", hotkeyThreadId);
    hookEnabled = 1;
    MH_EnableHook(MH_ALL_HOOKS);
  } else if (ul_reason_for_call == DLL_PROCESS_DETACH) {
    PostThreadMessageW(hotkeyThreadId, WM_USER_EXIT, 0, 0);
    WaitForSingleObject(hHotkeyThread, INFINITE);
    MH_DisableHook(MH_ALL_HOOKS);
    MH_Uninitialize();
  }
  return TRUE;
}
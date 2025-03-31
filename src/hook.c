#include "hook.h"

BOOL initHook(HookInstance *hook, void *function, void *listener) {
  hook->g_oldFuncAddr = function;

  // Store original code
  ReadProcessMemory(INVALID_HANDLE_VALUE, hook->g_oldFuncAddr, hook->g_oldCode, BYTES_TO_STORE, NULL);

  // Save new code
  DWORD64 offset = (DWORD64)listener;// - g_oldFuncAddr - 5;
  memset(hook->g_newCode, 0, BYTES_TO_STORE);
  hook->g_newCode[0] = 0xFF;
  hook->g_newCode[1] = 0x25;
  memcpy(&hook->g_newCode[6], &offset, 8);

  hook->trampoline = VirtualAlloc(NULL, BYTES_TO_STORE + 14, MEM_COMMIT, PAGE_EXECUTE_READWRITE);
  if (!hook->trampoline)
    return FALSE;
  memset(hook->trampoline, 0, BYTES_TO_STORE + 14);
  memcpy(hook->trampoline, hook->g_oldCode, BYTES_TO_STORE);
  ((char *)hook->trampoline)[BYTES_TO_STORE] = 0xFF;
  ((char *)hook->trampoline)[BYTES_TO_STORE] = 0x25;
  memcpy(&hook->trampoline[BYTES_TO_STORE + 6], hook->g_oldFuncAddr + BYTES_TO_STORE, 8);

  return TRUE;
}

BOOL installHook(HookInstance *hook) {
  DWORD oldProtect = 0;
  VirtualProtect((char *)hook->g_oldFuncAddr, BYTES_TO_STORE, PAGE_EXECUTE_READWRITE, &oldProtect);
  memcpy((char *)hook->g_oldFuncAddr, hook->g_newCode, BYTES_TO_STORE);
  VirtualProtect((char *)hook->g_oldFuncAddr, BYTES_TO_STORE, oldProtect, &oldProtect);
  return TRUE;
}

BOOL uninstallHook(HookInstance *hook) {
  DWORD oldProtect = 0;
  VirtualProtect((char *)hook->g_oldFuncAddr, BYTES_TO_STORE, PAGE_EXECUTE_READWRITE, &oldProtect);
  memcpy((char *)hook->g_oldFuncAddr, hook->g_oldCode, BYTES_TO_STORE);
  VirtualProtect((char *)hook->g_oldFuncAddr, BYTES_TO_STORE, oldProtect, &oldProtect);
  return TRUE;
}
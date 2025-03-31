#include <windows.h>

#define BYTES_TO_STORE 14

typedef struct {
  void *g_oldFuncAddr;
  void *trampoline;
  BYTE g_oldCode[BYTES_TO_STORE];
  BYTE g_newCode[BYTES_TO_STORE];
} HookInstance;

BOOL initHook(HookInstance *hook, void *function, void *listener);
BOOL installHook(HookInstance *hook);
BOOL uninstallHook(HookInstance *hook);
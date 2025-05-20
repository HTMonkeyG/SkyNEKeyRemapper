#include "dllmain.h"

// Hook listeners.
PeekMessage_t PeekMessageW_old, PeekMessageA_old;
i08 hookEnabled = 1
  , overlayShow = 0;
DWORD hotkeyThreadId;
HANDLE hHotkeyThread;
wchar_t currentSchemeName[MAX_PATH] = {0}
  , storedDataPath[MAX_PATH * 2] = {0}
  , schemeFolder[MAX_PATH * 2] = {0};
KeyRemapScheme defaultScheme = {0}
  , cachedScheme = {0}
  , *currentScheme = NULL;
Hotkey_t hkNext = {
    .mod = MOD_CONTROL,
    .vk = VK_F1
  }
  , hkEnable = {
    .mod = MOD_CONTROL,
    .vk = VK_F5
  };
OverlayWindow overlay = {0};

/* The variables below are set by switchScheme() function. */
// All .txt files under `schemes\` folder.
WIN32_FIND_DATAW *allFiles = NULL;
// Length of allFiles array.
u32 fileAmount = 0;
// Index in allFiles of current scheme. -1 indicates the default scheme.
i32 fileIndex = -1;
// 0 if current scheme is invalid.
i08 fileValid = 0;

BOOL PeekMessageW_Listener(
  LPMSG lpMsg,
  HWND hWnd,
  UINT wMsgFilterMin,
  UINT wMsgFilterMax,
  UINT wRemoveMsg
) {
  BOOL result;
  result = PeekMessageW_old(lpMsg, hWnd, wMsgFilterMin, wMsgFilterMax, wRemoveMsg);
  if (result && modifyKeyMsgWith(lpMsg, currentScheme) && overlayShow) {
    // Only when there is a real WM_KEY* message and the overlay is shown,
    // this function will be executed once.
    // So the performance will not be bad.
    setOverlayShow(&overlay, SW_HIDE);
    overlayShow = 0;
  }
  return result;
}

BOOL PeekMessageA_Listener(
  LPMSG lpMsg,
  HWND hWnd,
  UINT wMsgFilterMin,
  UINT wMsgFilterMax,
  UINT wRemoveMsg
) {
  BOOL result;
  result = PeekMessageA_old(lpMsg, hWnd, wMsgFilterMin, wMsgFilterMax, wRemoveMsg);
  if (result && modifyKeyMsgWith(lpMsg, currentScheme) && overlayShow) {
    setOverlayShow(&overlay, SW_HIDE);
    overlayShow = 0;
  }
  return result;
}

void cfgCallback(const wchar_t *key, const wchar_t *value, void *pUser) {
  i32 from, to, r, s;
  if (!wcscmp(key, L"hotkey_next"))
    // Next scheme.
    buildHotkeyFrom(value, &hkNext);
  else if (!wcscmp(key, L"hotkey_enable"))
    // Enable or disable the remapping.
    buildHotkeyFrom(value, &hkEnable);
  else {
    // Build default config.
    r = swscanf(key, L"%i", &from);
    s = swscanf(value, L"%i", &to);

    if (!r || !s || r == EOF || s == EOF)
      return;

    // Invalid Scan 1 Make.
    if (((from & 0xFF00) != 0xE000 && (from & 0xFF00))
      || ((to & 0xFF00) != 0xE000 && (to & 0xFF00)))
      return;

    // Storage remap config.
    if (from & 0xFF00)
      defaultScheme.extendedKeys[from & 0xFF] = to;
    else
      defaultScheme.normalKeys[from & 0xFF] = to;
    
    // Count the valid remappings.
    (*(i32 *)pUser)++;
  }
}

/** Create file paths with path to the dll. */
i32 initPaths(
  HMODULE hModule,
  wchar_t *configPath,
  wchar_t *dataPath,
  wchar_t *schemePath
) {
  wchar_t dllPath[MAX_PATH * 2] = {0}
    , *dir;

  if (!GetModuleFileNameW(hModule, dllPath, MAX_PATH))
    return 0;
  
  dir = wcsrchr(dllPath, L'\\');
  if (!dir)
    return 0;
  *dir = 0;

  wcscpy(configPath, dllPath);
  wcscat_s(configPath, MAX_PATH, L"\\skykey-config.txt");

  wcscpy(dataPath, dllPath);
  wcscat_s(dataPath, MAX_PATH, L"\\.skykey-store");

  wcscpy(schemePath, dllPath);
  wcscat_s(schemePath, MAX_PATH, L"\\schemes\\");

  return 1;
}

/** Save current states. */
void saveData() {
  wchar_t content[MAX_PATH + 2];
  size_t l;
  FILE *fd;

  // Contains the state flag and '\0' terminator.
  l = wcslen(currentSchemeName) + 2;
  fd = _wfopen(storedDataPath, L"wb");
  content[0] = (u16)hookEnabled;
  wcscpy(content + 1, currentSchemeName);
  fwrite(content, sizeof(wchar_t), l, fd);
  fclose(fd);
  log(L"Saved data at %s\n", storedDataPath);
}

/** Switch state of the hooks. */
void switchState() {
  if (hookEnabled) {
    hookEnabled = 0;
    MH_DisableHook(MH_ALL_HOOKS);
    log(L"Hook disabled.\n");
  } else {
    hookEnabled = 1;
    log(L"Hook enabled.\n");
    MH_EnableHook(MH_ALL_HOOKS);
  }
}

/** Switch to next scheme or reload current scheme. */
void switchScheme(i08 next) {
  wchar_t path[MAX_PATH * 2];
  FILE *fd;
  KeyRemapScheme temp;
  i08 found = 0
    , isDefault = !wcscmp(currentSchemeName, SCHEME_DEFAULT);
  
  if (!next && isDefault)
    // If the function is intended to read currentSchemeName, then directly
    // set to default scheme.
    goto SetDefault;

  wcscpy(path, schemeFolder);
  wcscat(path, L"*.txt");
  // Get all .txt files in schemes/ folder.
  free(allFiles);
  getAllFilesSorted(path, &allFiles, &fileAmount);
  if (!allFiles)
    // No .txt file, set to default.
    goto SetDefault;
  else for (u32 i = 0; i < fileAmount; i++) {
    // Check for whether the current file is exist.
    if (isDefault)
      // If current scheme is the default, then select the first file.
      goto SetScheme;
    else if (!wcscmp(currentSchemeName, allFiles[i].cFileName)) {
      // Find current file.
      if (!next)
        goto SetScheme;
      // Select the next file.
      found = 1;
      continue;
    }
    if (found) {
      // Choose the next file of current scheme.
SetScheme:
      // Read the file of current index.
      fileIndex = i;
      fileValid = 0;
      wcscpy(currentSchemeName, allFiles[i].cFileName);
      wcscpy(path, schemeFolder);
      wcscat_s(path, MAX_PATH, currentSchemeName);
      fd = _wfopen(path, L"r");

      // Failed to read or read an empty scheme.
      if (!fd)
        goto SetDefault;
      if (!buildSchemeFrom(fd, &temp)) {
        fclose(fd);
        goto SetDefault;
      }

      memcpy(&cachedScheme, &temp, sizeof(KeyRemapScheme));
      currentScheme = &cachedScheme;
      hookEnabled = 1;
      fileValid = 1;
      return;
    }
  }

SetDefault:
  // If no match or current file is the last file, then set to default.
  wcscpy(currentSchemeName, SCHEME_DEFAULT);
  fileIndex = -1;
  currentScheme = &defaultScheme;
}

/** Initialize the dll. */
void initDll(HMODULE hModule, i08 *hookEnabled) {
  wchar_t content[MAX_PATH] = {0}
    , configPath[MAX_PATH];
  FILE *fd;
  i32 validAmount = 0;

  if (!initPaths(hModule, configPath, storedDataPath, schemeFolder))
    return;

  // Read config file.
  fd = _wfopen(configPath, L"r");
  if (!fd)
    return;
  buildConfigFrom(fd, cfgCallback, (void *)&validAmount);
  fclose(fd);

  // Open stored data.
  fd = _wfopen(storedDataPath, L"rb");
  if (!fd) {
    // Seems like the first run, and set to default scheme.
    // The eight slashes specifies the default scheme, because of it can't
    // be used as any file name.
    wcscpy(currentSchemeName, SCHEME_DEFAULT);
    currentScheme = &defaultScheme;
    if (!validAmount)
      // If default scheme is empty, then disable the remapping.
      *hookEnabled = 0;
    return;
  }
  fread(content, sizeof(wchar_t), MAX_PATH - 1, fd);
  fclose(fd);

  // Restore previous state.
  *hookEnabled = !!content[0];
  // Restore previous scheme.
  wcscpy(currentSchemeName, content + 1);
  // Read previous scheme.
  switchScheme(FALSE);
}

/** Subthread for listening hotkey inputs. */
DWORD WINAPI hotkeyThread(LPVOID lpParam) {
  HINSTANCE hInstance = (HINSTANCE)lpParam;
  MSG msg;
  HWND gameWnd;
  DWORD minShowTimer = 0
    , maxShowTimer = 0;

  if (
    !registerHotkeyWith(NULL, 1, &hkEnable)
    || !registerHotkeyWith(NULL, 2, &hkNext)
  ) {
    log(L"Register hotkey failed.");
    return 1;
  }

  while (GetMessageW(&msg, NULL, 0, 0)) {
    // In most cases, the game window isn't created when the dll is injected,
    // so we don't find game window in initDll().
    gameWnd = FindWindowW(NULL, TARGET_WND_NAME);
    if (msg.message == WM_USER_EXIT)
      // Remove overlay window and terminate thread.
      PostQuitMessage(0);
    else if (msg.message == WM_QUIT)
      // Exit message loop.
      break;
    else if (msg.message == WM_HOTKEY && GetForegroundWindow() == gameWnd) {
      if (!overlay.window)
        createOverlay(&overlay, hInstance, gameWnd);

      if (msg.wParam == 1) {
        // Enable or disable.
        switchState();
        saveData();
      } else if (msg.wParam == 2) {
        // Next scheme.
        switchScheme(TRUE);
        saveData();
        log(L"%s\n", currentSchemeName);
      }

      // Update overlay.
      setOverlayShow(&overlay, SW_SHOWNOACTIVATE);
      updateOverlay(&overlay, allFiles, fileAmount, fileIndex, hookEnabled);

      // Set timers.
      // The window will continue to display at least until this timer is
      // triggered.
      minShowTimer = SetTimer(NULL, minShowTimer, 500, NULL);
      // The window will be displayed until this timer is triggered at most.
      maxShowTimer = SetTimer(NULL, maxShowTimer, 10000, NULL);
    } else if (msg.message == WM_TIMER) {
      if (msg.wParam == minShowTimer) {
        // Allow the hook to hide the overlay when any key is pressed.
        overlayShow = 1;
        KillTimer(NULL, minShowTimer);
      } else if (msg.wParam == maxShowTimer) {
        // Hide the overlay directly.
        overlayShow = 0;
        setOverlayShow(&overlay, SW_HIDE);
        KillTimer(NULL, maxShowTimer);
      }
    }
  }

  // Clear resources.
  removeOverlay(&overlay);
  UnregisterHotKey(NULL, 1);
  UnregisterHotKey(NULL, 2);
  return msg.wParam;
}

BOOL APIENTRY DllMain(
  HMODULE hModule,
  DWORD ul_reason_for_call,
  LPVOID lpReserved
) {
  if (ul_reason_for_call == DLL_PROCESS_ATTACH) {
    // Only for debug use.
    // Attach a console window to the game process.
#ifdef DEBUG_CONSOLE
    FreeConsole();
    AllocConsole();
    freopen("CONOUT$","w+t",stdout);
    freopen("CONIN$","r+t",stdin);
#endif
    log(L"Dll injected.\n");

    // Initialize MinHook.
    MH_Initialize();

    // Hook PeekMessage functions.
    // Sky:CotL uses these functions to check keyboard input.
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
    log(L"Hooked PeekMessage.\n");

    // Initialize dll.
    initDll(hModule, &hookEnabled);
    //createOverlay(&overlay, hModule);
    hHotkeyThread = CreateThread(
      NULL,
      0,
      hotkeyThread,
      (LPVOID)hModule,
      0,
      &hotkeyThreadId
    );
    if (hookEnabled)
      MH_EnableHook(MH_ALL_HOOKS);
  } else if (ul_reason_for_call == DLL_PROCESS_DETACH) {
    // Deinitialze.
    // Kill the subthread and disable all hooks.
    PostThreadMessageW(hotkeyThreadId, WM_USER_EXIT, 0, 0);
    WaitForSingleObject(hHotkeyThread, INFINITE);
    MH_DisableHook(MH_ALL_HOOKS);
    MH_Uninitialize();
    // Save data for next launch.
    saveData();
  }
  return TRUE;
}

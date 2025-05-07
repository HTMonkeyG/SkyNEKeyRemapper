#include "dllmain.h"

i08 hookEnabled = 1;
PeekMessage_t PeekMessageW_old, PeekMessageA_old;
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
    .vk = VK_F3
  }
  , hkEnable = {
    .mod = MOD_CONTROL,
    .vk = VK_F5
  };

static BOOL PeekMessageW_Listener(
  LPMSG lpMsg,
  HWND hWnd,
  UINT wMsgFilterMin,
  UINT wMsgFilterMax,
  UINT wRemoveMsg
) {
  BOOL result;
  result = PeekMessageW_old(lpMsg, hWnd, wMsgFilterMin, wMsgFilterMax, wRemoveMsg);
  if (result)
    modifyKeyMsgWith(lpMsg, currentScheme);
  return result;
}

static BOOL PeekMessageA_Listener(
  LPMSG lpMsg,
  HWND hWnd,
  UINT wMsgFilterMin,
  UINT wMsgFilterMax,
  UINT wRemoveMsg
) {
  BOOL result;
  result = PeekMessageA_old(lpMsg, hWnd, wMsgFilterMin, wMsgFilterMax, wRemoveMsg);
  if (result)
    modifyKeyMsgWith(lpMsg, currentScheme);
  return result;
}

static void cfgCallback(const wchar_t *key, const wchar_t *value, void *pUser) {
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
    s = swscanf(key, L"%i", &to);

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
static i32 initPaths(
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

/** Initialize the dll. */
static void initDll(HMODULE hModule) {
  wchar_t content[MAX_PATH] = {0}
    , configPath[MAX_PATH]
    , temp[MAX_PATH];
  FILE *fd;
  i32 validDefaultKey = 0;

  if (!initPaths(hModule, configPath, storedDataPath, schemeFolder))
    return;

  // Read config file.
  fd = _wfopen(configPath, L"r");
  if (!fd)
    return;
  buildConfigFrom(fd, cfgCallback, (void *)&validDefaultKey);
  fclose(fd);

  // Read memorized scheme name.
  fd = _wfopen(storedDataPath, L"rb");
  if (!fd)
    // Seems like the first run, and set to default scheme.
    goto SetToDefault;
  fread(content, sizeof(wchar_t), MAX_PATH - 1, fd);
  fclose(fd);

  // Restore previous state.
  hookEnabled = !!content[0];
  // Restore previous scheme.
  wcscpy(currentSchemeName, content + 1);
  if (!wcscmp(currentSchemeName, SCHEME_DEFAULT))
    goto CheckDefault;

  // Read previous scheme.
  // Cached scheme folder has the trailing backslash.
  wcscpy(temp, schemeFolder);
  wcscat_s(temp, MAX_PATH, currentSchemeName);
  fd = _wfopen(temp, L"rb");
  if (!fd)
    // If previous scheme is unexist, then set to default.
    goto SetToDefault;

  if (!buildSchemeFrom(fd, &cachedScheme)) {
    // Empty scheme.
SetToDefault:
    // The eight slashes specifies the default scheme, because of it can't
    // be used as any file name.
    wcscpy(currentSchemeName, SCHEME_DEFAULT);
    currentScheme = &defaultScheme;
CheckDefault:
    if (!validDefaultKey)
      // If default scheme is empty, then disable the remapping.
      hookEnabled = 0;
    return;
  }
  fclose(fd);
  currentScheme = &cachedScheme;
}

/** Save current states. */
static void saveData() {
  wchar_t content[MAX_PATH + 2];
  size_t l;
  FILE *fd;

  // Contains the state flag and '\0' terminator.
  l = wcslen(currentSchemeName) + 2;
  wprintf(L"%llu\n", l);
  fd = _wfopen(storedDataPath, L"wb");
  content[0] = (u16)hookEnabled;
  wcscpy(content + 1, currentSchemeName);
  fwrite(content, sizeof(wchar_t), l, fd);
  fclose(fd);
}

/** Switch state of the hooks. */
static void switchState() {
  if (hookEnabled) {
    hookEnabled = 0;
    MH_DisableHook(MH_ALL_HOOKS);
    printf("Hook disabled.\n");
  } else {
    hookEnabled = 1;
    printf("Hook enabled.\n");
    MH_EnableHook(MH_ALL_HOOKS);
  }
}

/** Switch to next scheme. */
static void switchScheme() {
  HANDLE hFind;
  WIN32_FIND_DATAW findData;
  wchar_t path[MAX_PATH * 2];
  FILE *fd;
  KeyRemapScheme temp;
  i08 found = 0;

  wcscpy(path, schemeFolder);
  wcscat(path, L"*.txt");
  hFind = FindFirstFileW(path, &findData);
  if (!hFind)
    return;
  else do {
    if (
      !wcscmp(findData.cFileName, L".")
      || !wcscmp(findData.cFileName, L"..")
    )
      continue;
    if (!wcscmp(currentSchemeName, SCHEME_DEFAULT)) {
      // If default, then select the first file.
      wcscpy(currentSchemeName, findData.cFileName);
      goto SetScheme;
    } else if (wcscmp(currentSchemeName, findData.cFileName)) {
      // Found current file, prepared for next file.
      found = 1;
      continue;
    }
    if (found) {
      // Choose the next file of current scheme.
SetScheme:
      wcscpy(currentSchemeName, findData.cFileName);
      wcscpy(path, schemeFolder);
      wcscat_s(path, MAX_PATH, currentSchemeName);
      fd = _wfopen(path, L"r");
      if (!fd || !buildSchemeFrom(fd, &temp))
        // Read failed or empty scheme.
        return;
      memcpy(&cachedScheme, &temp, sizeof(KeyRemapScheme));
      currentScheme = &cachedScheme;
      hookEnabled = 1;
      return;
    }
  } while (FindNextFileW(hFind, &findData));
  // If no match or current file is the last file, then set to default.
  wcscpy(currentSchemeName, SCHEME_DEFAULT);
  currentScheme = &defaultScheme;
}

/** Subthread for listening hotkey inputs. */
static DWORD WINAPI hotkeyThread(LPVOID lpParam) {
  MSG msg;

  if (
    !registerHotkeyWith(NULL, 1, &hkEnable)
    || !registerHotkeyWith(NULL, 2, &hkNext)
  ) {
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
    else if (
      msg.message != WM_HOTKEY
      || GetForegroundWindow() != FindWindowW(NULL, L"光·遇")
    )
      // Ignore other messages.
      continue;
    else if (msg.message == WM_HOTKEY && msg.wParam == 1) {
      // Enable or disable.
      switchState();
      saveData();
    } else if (msg.message == WM_HOTKEY && msg.wParam == 2) {
      // Next scheme.
      switchScheme();
      saveData();
      wprintf(L"%s\n", currentSchemeName);
    }
  }

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
    //FreeConsole();
    //AllocConsole();
    //freopen("CONOUT$","w+t",stdout);
    //freopen("CONIN$","r+t",stdin);
    printf("DLL injected\n");
#endif

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

    // Initialize dll.
    initDll(hModule);
    hHotkeyThread = CreateThread(NULL, 0, hotkeyThread, 0, 0, &hotkeyThreadId);
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
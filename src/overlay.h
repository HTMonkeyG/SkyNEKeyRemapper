#include <windows.h>
#include <stdio.h>
#include "macros.h"

#define UPDATE_FREQ 20
#define FONT_HEIGHT 48
#define NAME_DEFAULT L"Default"
#define COLOR_1 RGB(102, 51, 0)
#define COLOR_2 RGB(255, 140, 26)

typedef struct {
  HANDLE subThread;
  HWND window;
  HWND target;
  HFONT font;
  DWORD threadId;
  wchar_t (*text)[MAX_PATH];
  i32 lastShow;
  i08 enabled;
} OverlayWindow;

i32 createOverlay(
  OverlayWindow *overlay,
  HINSTANCE hInstance,
  HWND targetWindow
);
i32 updateOverlay(
  OverlayWindow *overlay,
  WIN32_FIND_DATAW *files,
  u32 length,
  i32 index,
  i08 enabled
);
i32 tickOverlay(OverlayWindow *overlay);
i32 setOverlayShow(OverlayWindow *overlay, int nCmdShow);
i32 removeOverlay(OverlayWindow *overlay);

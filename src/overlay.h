#include <windows.h>
#include <stdio.h>
#include "macros.h"

#define UPDATE_FREQ 20
#define WND_HEIGHT 180
#define FONT_HEIGHT 48
#define NAME_DEFAULT L"Default"
#define COLOR_1 RGB(102, 51, 0)
#define COLOR_2 RGB(255, 140, 26)
#define CreateFontMinifyW(font, height) (CreateFontW( \
  height, \
  0, \
  0, \
  0, \
  FW_NORMAL, \
  FALSE, \
  FALSE, \
  FALSE, \
  DEFAULT_CHARSET, \
  OUT_OUTLINE_PRECIS, \
  CLIP_DEFAULT_PRECIS, \
  NONANTIALIASED_QUALITY, \
  FIXED_PITCH | FF_MODERN, \
  font \
))

typedef struct {
  HINSTANCE hInstance;
  HANDLE subThread;
  HANDLE hEvent;
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
i32 setOverlayShow(OverlayWindow *overlay, int nCmdShow);
i32 removeOverlay(OverlayWindow *overlay);

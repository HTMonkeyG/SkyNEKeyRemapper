#include "overlay.h"

static void drawTextWithShadow(
  HDC hdc,
  wchar_t *string,
  RECT *rect,
  i08 type
) {
  RECT temp;
  HBRUSH hBr;
  HPEN hPen;
  HGDIOBJ hOld;

  if (!type) {
    // Normal. Solid text with shadow.
    temp.bottom = rect->bottom + 1;
    temp.top = rect->top + 1;
    temp.left = rect->left + 1;
    temp.right = rect->right + 1;
    // The shadow of the text.
    SetTextColor(hdc, COLOR_1);
    DrawTextW(hdc, string, -1, &temp, DT_CENTER | DT_VCENTER | DT_SINGLELINE);
    // The text. Overlapped the shadow.
    SetTextColor(hdc, COLOR_2);
    DrawTextW(hdc, string, -1, rect, DT_CENTER | DT_VCENTER | DT_SINGLELINE);
  } else if (type == 1) {
    // Inverted. Transparent text in solid background.
    // Fill solid background.
    hBr = CreateSolidBrush(COLOR_2);
    FillRect(hdc, rect, hBr);
    DeleteObject(hBr);
    // Shadow of the background.
    BeginPath(hdc);
    MoveToEx(hdc, rect->left + 1, rect->bottom - 1, 0);
    LineTo(hdc, rect->right - 1, rect->bottom - 1);
    LineTo(hdc, rect->right - 1, rect->top);
    EndPath(hdc);
    hPen = CreatePen(PS_SOLID, 1, COLOR_1);
    hOld = SelectObject(hdc, hPen);
    StrokePath(hdc);
    SelectObject(hdc, hOld);
    DeleteObject(hPen);
    // Transparent text.
    SetTextColor(hdc, 0);
    DrawTextW(hdc, string, -1, rect, DT_CENTER | DT_VCENTER | DT_SINGLELINE);
  } else if (type == 3) {
    // Bordered. Solid text with shadow, and a solid border.
  }
}

/** Move the overlay to the target window. */
static void setOverlayPos(OverlayWindow *overlay) {
  RECT rect;
  int x, y, xL, yL;
  const int WND_HEIGHT = 180;

  if (!overlay || !overlay->window || !overlay->target)
    return;
  
  if (
    GetForegroundWindow() != overlay->target
    && overlay->lastShow != SW_HIDE
  ) {
    setOverlayShow(overlay, SW_HIDE);
    return;
  }

  if (GetWindowRect(overlay->target, &rect)) {
    x = rect.left;
    y = rect.top + (rect.bottom - rect.top - WND_HEIGHT) / 2;
    xL = rect.right - rect.left;
    yL = WND_HEIGHT;
    SetWindowPos(
      overlay->window,
      HWND_TOPMOST,
      x, y, xL, yL,
      SWP_NOACTIVATE | SWP_NOREDRAW
    );
  }
}

static i32 tickOverlay(OverlayWindow *overlay) {
  if (!overlay || !overlay->window)
    return 0;
  setOverlayPos(overlay);
  InvalidateRect(overlay->window, NULL, FALSE);
  UpdateWindow(overlay->window);
  return 1;
}

static DWORD WINAPI ovWndThread(LPVOID lpParam) {
  OverlayWindow *overlay = (OverlayWindow *)lpParam;
  MSG msg;

  overlay->font = CreateFontMinifyW(L"Consolas", FONT_HEIGHT);

  if (!overlay->font)
    goto SetEventAndReturn;
  
  // Create overlay window.
  overlay->window = CreateWindowExW(
    WS_EX_LAYERED | WS_EX_TRANSPARENT | WS_EX_TOPMOST | WS_EX_NOACTIVATE,
    L"SkyKeyOverlay",
    NULL,
    WS_POPUP,
    0, 0, 0, 0,
    NULL,
    NULL,
    overlay->hInstance,
    NULL
  );
  
  if (!overlay->window)
    goto SetEventAndReturn;

  SetEvent(overlay->hEvent);
  SetLayeredWindowAttributes(overlay->window, RGB(0, 0, 0), 0, LWA_COLORKEY);
  SetTimer(NULL, 1, 1000 / UPDATE_FREQ, NULL);

  // Message loop.
  while (GetMessageW(&msg, NULL, 0, 0)) {
    if (msg.message == WM_QUIT)
      break;
    else if (msg.message == WM_TIMER)
      tickOverlay(overlay);
    TranslateMessage(&msg);
    DispatchMessageW(&msg);
  }

  return 0;

SetEventAndReturn:
  SetEvent(overlay->hEvent);
  return 0;
}

static LRESULT CALLBACK ovWndProc(
  HWND hWnd,
  UINT message,
  WPARAM wParam,
  LPARAM lParam
) {
  OverlayWindow *overlay;
  HDC hdc;
  PAINTSTRUCT ps;
  HBRUSH hBrush;
  RECT rcClient, rText;
  i32 height;

  overlay = (OverlayWindow *)GetWindowLongPtrW(hWnd, GWLP_USERDATA);
  if (!overlay)
    goto RunDefWndProc;

  if (message == WM_PAINT) {
    hdc = BeginPaint(hWnd, &ps);
    hBrush = CreateSolidBrush(0);

    // Clear screen.
    GetClientRect(hWnd, &rcClient);
    FillRect(hdc, &rcClient, hBrush);
    DeleteObject(hBrush);
    SetBkMode(hdc, TRANSPARENT);

    // Draw 3 lines of text.
    SelectObject(hdc, overlay->font);
    height = rcClient.bottom / 3;
    rText.top = 0;
    rText.bottom = height;
    rText.left = 0;
    rText.right = rcClient.right;
    for (i32 yC = 0; yC < 3; yC++) {
      drawTextWithShadow(hdc, overlay->text[yC], &rText, yC == 1 ? 1 : 0);
      rText.bottom += height;
      rText.top += height;
    }

    // Done.
    EndPaint(hWnd, &ps);
    return 0;
  } else if (message == WM_DESTROY) {
    // Terminate window thread.
    PostQuitMessage(0);
    return 0;
  }

RunDefWndProc:
  return DefWindowProcW(hWnd, message, wParam, lParam);
}

/** Create overlay window and subthread. */
i32 createOverlay(
  OverlayWindow *overlay,
  HINSTANCE hInstance,
  HWND targetWindow
) {
  WNDCLASSEXW wc = {0};
  static ATOM wca = 0;

  if (!overlay || !hInstance || !targetWindow)
    return 0;

  if (!wca) {
    // Create window class.
    wc.cbSize = sizeof(WNDCLASSEXW);
    wc.style = CS_HREDRAW | CS_VREDRAW;
    wc.lpfnWndProc = ovWndProc;
    wc.hInstance = hInstance;
    wc.hCursor = LoadCursor(NULL, IDC_ARROW);
    wc.lpszClassName = L"SkyKeyOverlay";
    wc.hbrBackground = (HBRUSH)CreateSolidBrush(RGB(0, 0, 0));

    wca = RegisterClassExW(&wc);
    if (!wca)
      return 0;
  }
  overlay->hInstance = hInstance;
  // We need to wait for the window's creation, so this event is
  // created.
  overlay->hEvent = (void *)CreateEventW(NULL, TRUE, FALSE, NULL);

  overlay->subThread = CreateThread(
    NULL,
    0,
    ovWndThread,
    (LPVOID)overlay,
    0,
    &overlay->threadId
  );
  if (!overlay->subThread)
    return 0;
  
  WaitForSingleObject(overlay->hEvent, INFINITE);

  overlay->target = targetWindow;
  overlay->text = malloc(3 * MAX_PATH * sizeof(wchar_t));

  return 1;
}

i32 updateOverlay(
  OverlayWindow *overlay,
  WIN32_FIND_DATAW *files,
  u32 length,
  i32 index,
  i08 enabled
) {
  if (!overlay || !files || !overlay->text)
    return 0;

  memset(overlay->text, 0, 3 * MAX_PATH * sizeof(wchar_t));

  for (i32 i = index - 1, j = 0; j < 3; i++, j++) {
    if (i == -1)
      // Default scheme.
      // When index == -1, then the first row won't display anything, the
      // Second row will display "Default". If there's other schemes, then
      // display them below.
      // When index > -1, the first row is "Default", the other is file names.
      wcscpy(overlay->text[j], NAME_DEFAULT);
    else if (i < length)
      wcscpy(overlay->text[j], files[i].cFileName);
  }

  overlay->enabled = enabled;

  // Pass struct overlay to window process.
  SetWindowLongPtrW(overlay->window, GWLP_USERDATA, (LONG_PTR)overlay);
  tickOverlay(overlay);

  return 1;
}

i32 setOverlayShow(OverlayWindow *overlay, int nCmdShow) {
  if (!overlay)
    return 0;
  ShowWindow(overlay->window, nCmdShow);
  overlay->lastShow = nCmdShow;
  return 1;
}

i32 removeOverlay(OverlayWindow *overlay) {
  if (!overlay || !overlay->window || !overlay->subThread || !overlay->font)
    return 0;
  DestroyWindow(overlay->window);
  WaitForSingleObject(overlay->subThread, INFINITE);
  DeleteObject((HGDIOBJ)overlay->font);
  //ReleaseMutex(overlay->mutex);
  free(overlay->text);
  memset(overlay, 0, sizeof(OverlayWindow));
  return 1;
}

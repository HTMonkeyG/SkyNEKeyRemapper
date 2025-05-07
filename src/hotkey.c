#include "keys.h"
#include "hotkey.h"

#include <stdio.h>

static i32 wcsniacmp(const wchar_t *s1, const wchar_t *s2, size_t max) {
  wchar_t c1, c2, lc1, lc2;
  int ac1, ac2;

  while (max > 0) {
    c1 = *s1;
    c2 = *s2;

    ac1 = (c1 != L'\0' && iswalnum(c1));
    ac2 = (c2 != L'\0' && iswalnum(c2));

    if (!ac1 || !ac2) {
      if (!ac1 && !ac2)
        return 0;
      else if (!ac1)
        return -1;
      else
        return 1;
    }

    lc1 = towlower(c1);
    lc2 = towlower(c2);

    if (lc1 != lc2)
      return (lc1 < lc2) ? -1 : 1;
    
    s1++;
    s2++;
    max--;
  }

  return 0;
}

i32 getKeyFromName(const wchar_t *name, const wchar_t **endp) {
  size_t l, m;
  i32 result;

  if (!name)
    return 0;

  m = wcslen(name) + 1;
  for (u08 i = 0; i < VK_ARRAY_LENGTH; i++) {
    if (!ALIASES[i])
      continue;

    l = wcslen(ALIASES[i]) + 1;
    if (l <= m && !wcsniacmp(ALIASES[i], name, l)) {
      // Bit[7:0], keycode.
      result = (i32)i & VK_LMASK;

      // Bit[15:8], modifier flag.
      if (i == VK_SHIFT || i == VK_LSHIFT || i == VK_RSHIFT)
        result |= VK_MODSHIFT;
      if (i == VK_CONTROL || i == VK_LCONTROL || i == VK_RCONTROL)
        result |= VK_MODCTRL;
      if (i == VK_MENU || i == VK_LMENU || i == VK_RMENU)
        result |= VK_MODALT;
      if (i == VK_LWIN || i == VK_RWIN)
        result |= VK_MODWIN;

      if (endp)
        // End reading.
        *endp = name + l;
      return result;
    }
  }

  if (endp)
    // End reading.
    *endp = name;
  return 0;
}

i32 buildHotkeyFrom(const wchar_t *desc, Hotkey_t *hk) {
  const wchar_t *p = desc
    , *q, *e;
  i32 mod = 0
    , vk = 0
    , lastKey, t;

  if (!desc || !hk)
    return 0;

  e = p + wcslen(desc);

  while (p < e) {
    // Skip whitespace, only for ASCII characters.
    while (*p == L' ' || *p == L'\t' || *p == L'+')
      p++;

    t = getKeyFromName(p, &q);
    if (!t)
      // Invalid hotkey descriptor, discard and return 0.
      // Maybe return previously parsed result would be better?
      return 0;
    p = q;
    
    if (t & VK_HMASK)
      mod |= (t >> 8) & 0xFF;
    else
      vk = t & VK_LMASK;
    
    lastKey = t;
  }

  if (!vk) {
    // Allow hotkeys like Ctrl+Shift.
    // The last key would be considered as vk.
    vk = lastKey & VK_LMASK;
    // Avoiding repeated keys like Shift+Shift+Shift (LOL).
    mod &= ~((lastKey >> 8) & 0xFF);
    if (!mod)
      // But it is a bad sequence when only specified single modifier key.
      return 0;
  }

  hk->mod = mod;
  hk->vk = vk;

  return 1;
}

i32 registerHotkeyWith(HWND hWnd, int id, const Hotkey_t *hk) {
  if (!hk || !hk->vk)
    return 0;
  return RegisterHotKey(hWnd, id, hk->mod, hk->vk);
}

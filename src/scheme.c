#include "scheme.h"

void modifyKeyMsgWith(LPMSG lpMsg, KeyRemapScheme *scheme) {
  u08 scanCode, isExtended;
  u16 *config;
  if (!lpMsg || !scheme)
    return;

  if (lpMsg->message == WM_KEYDOWN || lpMsg->message == WM_KEYUP) {
    // Get scan code of the key.
    scanCode = (lpMsg->lParam & 0x00FF0000) >> 16;
    isExtended = (lpMsg->lParam & 0x01000000) >> 24;
    config = isExtended ? scheme->extendedKeys : scheme->normalKeys;
    if (config[scanCode]) {
      // Do remapping.
      // Clear scan code and extended flag.
      lpMsg->lParam &= (~0x01FF0000);
      if (config[scanCode] == 0xFFFF)
        // The key is disabled, let it be zero.
        return;
      // Set scan code.
      lpMsg->lParam |= (config[scanCode] & 0xFF) << 16;
      // Set extended flag.
      (config[scanCode] & 0xFF00) && (lpMsg->lParam |= 0x01000000);
    }
  }
}

static void buildCallback(const wchar_t *key, const wchar_t *value, void *pUser) {
  KeyRemapScheme *scheme = (KeyRemapScheme *)pUser;
  i32 from, to, r, s;

  r = swscanf(key, L"%i", &from);
  s = swscanf(value, L"%i", &to);

  if (!r || !s || r == EOF || s == EOF)
    return;

  // Invalid Scan 1 Make.
  if (((from & 0xFF00) != 0xE000 && (from & 0xFF00))
    || ((to & 0xFF00) != 0xE000 && to != 0xFFFF && (to & 0xFF00)))
    return;

  // Storage remap config.
  // Disable this key by set map result to 0xFFFF.
  if (from & 0xFF00)
    scheme->extendedKeys[from & 0xFF] = to;
  else
    scheme->normalKeys[from & 0xFF] = to;
}

i32 buildSchemeFrom(FILE *file, KeyRemapScheme *scheme) {
  memset(scheme, 0, sizeof(KeyRemapScheme));
  return buildConfigFrom(file, buildCallback, (void *)scheme);
}
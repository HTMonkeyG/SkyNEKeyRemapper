#include "scheme.h"

i08 modifyKeyMsgWith(LPMSG lpMsg, KeyRemapScheme *scheme) {
  u08 scanCode, isExtended, result;
  u16 *config;

  if (!lpMsg || !scheme)
    return 0;

  if (lpMsg->message == WM_KEYDOWN || lpMsg->message == WM_KEYUP) {
    result = lpMsg->message == WM_KEYDOWN;
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
        return result;
      // Set scan code.
      lpMsg->lParam |= (config[scanCode] & 0xFF) << 16;
      // Set extended flag.
      (config[scanCode] & 0xFF00) && (lpMsg->lParam |= 0x01000000);
    }
    return result;
  }
  return 0;
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

static int compare(const void *a, const void *b) {
  WIN32_FIND_DATAW *fd1 = (WIN32_FIND_DATAW *)a
    , *fd2 = (WIN32_FIND_DATAW *)b;
  return wcscmp((wchar_t *)fd1->cFileName, (wchar_t *)fd2->cFileName);
}

void getAllFilesSorted(
  const wchar_t *fileName,
  WIN32_FIND_DATAW **result,
  u32 *resultLen
) {
  WIN32_FIND_DATAW *found, *p
    , findData;
  HANDLE hFind;
  u32 counter = 0
    , maxSize = 4;

  *result = NULL;
  found = malloc(sizeof(WIN32_FIND_DATAW) * maxSize);
  // Saved data when first call FindFirstFileW, so set counter to 1.
  hFind = FindFirstFileW(fileName, &findData);
  if (!hFind) {
    free(found);
    return;
  } else {
    do {
      if (
        !wcscmp(findData.cFileName, L".")
        || !wcscmp(findData.cFileName, L"..")
      )
        continue;
      if (counter == maxSize) {
        // Push back.
        p = malloc(sizeof(WIN32_FIND_DATAW) * maxSize * 2);
        memcpy(p, found, sizeof(WIN32_FIND_DATAW) * maxSize);
        maxSize *= 2;
        free(found);
        found = p;
      }
      memcpy(found + counter, &findData, sizeof(WIN32_FIND_DATAW));
      counter++;
    } while (FindNextFileW(hFind, &findData));
  }
  if (!counter)
    return;

  qsort(found, counter, sizeof(WIN32_FIND_DATAW), compare);
  *result = found;
  *resultLen = counter;
}

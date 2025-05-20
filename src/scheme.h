#include <windows.h>
#include <stdio.h>

#include "macros.h"
#include "config.h"

typedef struct {
  u16 normalKeys[256];
  u16 extendedKeys[256];
} KeyRemapScheme;

i08 modifyKeyMsgWith(LPMSG lpMsg, KeyRemapScheme *scheme);
int buildSchemeFrom(FILE *file, KeyRemapScheme *scheme);
void getAllFilesSorted(
  const wchar_t *fileName,
  WIN32_FIND_DATAW **result,
  u32 *resultLen
);

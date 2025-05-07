#include <windows.h>
#include <stdio.h>

#include "macros.h"
#include "config.h"

typedef struct {
  u16 normalKeys[256];
  u16 extendedKeys[256];
} KeyRemapScheme;

void modifyKeyMsgWith(LPMSG lpMsg, KeyRemapScheme *scheme);
int buildSchemeFrom(FILE *file, KeyRemapScheme *scheme);

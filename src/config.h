#include "stdio.h"

unsigned short normalKeys[256]
  , extendedKeys[256];

int buildConfigFrom(FILE *file);
#include "config.h"

int buildConfigFrom(FILE *file) {
  int counter = 0, from, to;
  char line[128];
  while (!feof(file)) {
    fgets(line, 128, file);
    // Process comments
    if (line[0] == '#')
      continue;
    // Invalid line
    if (sscanf(line, "0x%x:0x%x", &from, &to) == EOF)
      continue;
    // Invalid OEM scan code
    if (((from & 0xFF00 != 0xE000) && (from & 0xFF00))
      || ((to & 0xFF00 != 0xE000) && (to & 0xFF00)))
      continue;

    // Storage remap
    if (from & 0xFF00)
      extendedKeys[from & 0xFF] = to;
    else
      normalKeys[from & 0xFF] = to;
    
    counter++;
  }

  return counter;
}
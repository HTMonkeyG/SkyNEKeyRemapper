#include "config.h"

int buildConfigFrom(FILE *file, ConfigCallback_t callback, void *pUser) {
  int counter = 0;
  wchar_t buffer[128]
    , *line, *o, *p, *q;
  while (!feof(file)) {
    fgetws(buffer, 128, file);
    o = buffer + wcslen(buffer) - 1;
    // Cut the line feed and space after the line.
    while (buffer <= o && (*o == L'\n' || *o == L'\r' || *o == L'\t' || *o == L' '))
      *o = 0, o--;
    line = buffer;
    // Skip whitespace.
    while (*line == L'\t' || *line == L' ')
      line++;
    // Process comments.
    if (line[0] == L'#' || !line[0])
      continue;
    p = wcschr(line, L':');
    // Invalid line.
    if (p == NULL)
      continue;
    // Trim the space after the key.
    q = p - 1;
    while (q > line && (*q == L'\t' || *q == L' '))
      *q = 0, q--;
    // Set line terminator of the key.
    *p = 0;
    p++;
    // Trim the space before the value.
    while (*p == L'\t' || *p == L' ' || *p == L'\r')
      p++;
    if (*p == L'\n')
      // Empty value.
      callback(line, L"", pUser);
    else
      callback(line, p, pUser);
    counter++;
  }

  return counter;
}

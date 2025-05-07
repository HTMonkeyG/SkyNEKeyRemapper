#ifndef __CONFIG_H__
#define __CONFIG_H__

#include <stdio.h>
#include <string.h>
#include <windows.h>

typedef void (ConfigCallback_t)(const wchar_t *key, const wchar_t *value, void *pUser);

int buildConfigFrom(FILE *file, ConfigCallback_t callback, void *pUser);

#endif
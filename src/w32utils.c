#ifdef WIN32

#define WIN32_LEAN_AND_MEAN
#define VC_EXTRALEAN

/* for GetLongPathName */
#define _WIN32_WINDOWS 0x0410

#include <windows.h>

#include "w32utils.h"
#include "alloc.h"

char *shortpath(const char *path)
{
    /* use satic buffers to simplify the code */
    static char short_path[MAX_PATH + 1];
    int ret = GetShortPathName(path, short_path, sizeof(short_path));
    if (!ret || ret > sizeof(short_path)) /* failed during the conversion */
	strcpy(short_path, path);
    return short_path;
}

char *longpath(const char *path)
{
    static char long_path [MAX_PATH + 1];
    int ret = GetLongPathName(path, long_path, sizeof(long_path));
    if (!ret || ret > sizeof(long_path))
	strcpy(long_path, path);
    return long_path;
}

void sleep(int sec)
{
  Sleep(sec * 1000);
}

#endif

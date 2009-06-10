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
    int path_len = strlen(path);
    char *short_path = mymalloc(path_len + 1);
    int ret = GetShortPathName(path, short_path, path_len + 1);
    if (ret <= path_len)
	return short_path;
    else /* failed during conversion */
	return strcpy(short_path, path);
}

char *longpath(const char *path)
{
    char *long_path = mymalloc(MAX_PATH + 1);
    int ret = GetLongPathName(path, long_path, MAX_PATH + 1);
    if (ret <= MAX_PATH)
	return long_path;
    else /* failed during conversion */
	return strcpy(long_path, path);
}

void sleep(int sec)
{
  Sleep(sec * 1000);
}

#endif

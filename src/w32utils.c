#include "w32utils.h"

#ifdef WIN32

#define WIN32_LEAN_AND_MEAN
#define VC_EXTRALEAN

/* for GetLongPathName */
#define _WIN32_WINDOWS 0x0410

#include <windows.h>

#include "alloc.h"

static char *get_longpath_internal(char *path, int len);

char *get_shortpath(char *path)
{
    if (strchr(path, ' ') == NULL) /* get short name only for files with spaces in names */
	return path;
    else
    {
	/* use satic buffers for simplicity */
	static char short_path[MAX_PATH + 1];
	int ret = GetShortPathName(path, short_path, sizeof(short_path));
	if (!ret || ret > sizeof(short_path))
	    /* error during the conversion */
	    return path;
	return short_path;
    }
}

void to_longpath(char *path, int len)
{
    char *long_path = get_longpath_internal(path, len);
    if (long_path != path)
	/* get_longpath_internal gurantees result will be no longer than len */
	strcpy(path, long_path);
}

char *get_longpath(char *path)
{
    return get_longpath_internal(path, -1);
}

static char *get_longpath_internal(char *path, int len)
{
    static char long_path[MAX_PATH + 1];
    int buflen;
    buflen = (len == -1 ? sizeof(long_path) : len);
    int ret = GetLongPathName(path, long_path, buflen);
    if (!ret || ret > buflen)
	/* error during the conversion */
	return path;
    return long_path;
}

void sleep(int sec)
{
    Sleep(sec * 1000);
}

#else
char *get_shortpath(char *path)
{
    return path;
}

void to_longpath(char *path, int len)
{
    NULL;
}

char *get_longpath(char *path)
{
    return path;
}
#endif /* WIN32 */

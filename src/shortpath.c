#ifdef WIN32

#define WIN32_LEAN_AND_MEAN
#define VC_EXTRALEAN

#include <windows.h>

#include "shortpath.h"
#include "alloc.h"

char *shortpath(const char *path)
{
    /* use short file paths
     * so it should work with long file paths 
     * even if -X command-line option was specified */
    int path_len = strlen(path);
    char *short_path = mymalloc(path_len + 1);
    int ret = GetShortPathName(path, short_path, path_len + 1);
    if (ret <= path_len)
	return short_path;
    else /* failed during conversion */
	return strcpy(short_path, path);
}
#endif

#ifndef CSCOPE_W32UTILS_H
#define CSCOPE_W32UTILS_H

#ifdef WIN32
char *get_shortpath(char *path);
char *get_longpath(char *path);
/*in-place conversion */
void to_longpath(char *path, int len);
void sleep(int sec);
#endif

#endif /* CSCOPE_W32UTILS_H */

#ifndef UTIL_H
#define UTIL_H

#include <sys/types.h>

char **rematch(char *pattern, char *string);
char *strip(char *s, char c);
int mkdeepdir(const char *path, mode_t mode);
int mkdeepdir_r(const char *path, mode_t mode, char **last);

#endif

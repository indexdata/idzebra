/*
 * Copyright (c) 1997, Index Data.
 * See the file LICENSE for details.
 * Sebastian Hammer, Adam Dickmeiss
 *
 * $Log: direntz.h,v $
 * Revision 1.1  1997-09-09 13:38:03  adam
 * Partial port to WIN95/NT.
 *
 *
 */
#ifdef WINDOWS
#include <windows.h>
struct dirent {
    char d_name[MAX_PATH+1];
};

typedef struct DIR DIR;

DIR *opendir (const char *path);
struct dirent *readdir (DIR *dd);
void closedir (DIR *dd);
#else
#include <dirent.h>
#endif

/*
 * Copyright (c) 1997-1999, Index Data.
 * See the file LICENSE for details.
 * Sebastian Hammer, Adam Dickmeiss
 *
 * $Log: direntz.h,v $
 * Revision 1.3  1999-02-02 14:50:33  adam
 * Updated WIN32 code specific sections. Changed header.
 *
 * Revision 1.2  1997/09/17 12:19:09  adam
 * Zebra version corresponds to YAZ version 1.4.
 * Changed Zebra server so that it doesn't depend on global common_resource.
 *
 * Revision 1.1  1997/09/09 13:38:03  adam
 * Partial port to WIN95/NT.
 *
 *
 */


#ifdef WIN32
/* make own version of dirent */
#include <windows.h>
struct dirent {
    char d_name[MAX_PATH];
};

typedef struct DIR DIR;

DIR *opendir (const char *path);
struct dirent *readdir (DIR *dd);
void closedir (DIR *dd);
#else
/* include UNIX version */
#include <dirent.h>
#endif

/*
 * Copyright (C) 1997-1999, Index Data
 * All rights reserved.
 * Sebastian Hammer, Adam Dickmeiss
 *
 * $Log: dirent.c,v $
 * Revision 1.3  1999-02-02 14:51:38  adam
 * Updated WIN32 code specific sections. Changed header.
 *
 * Revision 1.2  1997/09/17 12:19:24  adam
 * Zebra version corresponds to YAZ version 1.4.
 * Changed Zebra server so that it doesn't depend on global common_resource.
 *
 * 
 *
 * This utility implements a opendir/readdir/close on Windows.
 */

#ifdef WIN32
#include <assert.h>
#include <io.h>
#include <string.h>
#include <stdlib.h>

#include <direntz.h>

struct DIR {
    HANDLE handle;
    WIN32_FIND_DATA find_data;
    struct dirent entry;
};

DIR *opendir (const char *name)
{
    char fullName[MAX_PATH+1];
    DIR *dd = malloc (sizeof(*dd));

    if (!dd)
        return NULL;
    strcpy (fullName, name);
    strcat (fullName, "\\*.*");
    dd->handle = FindFirstFile(fullName, &dd->find_data);
    return dd;
}
                                                          
struct dirent *readdir (DIR *dd)
{
    if (dd->handle == INVALID_HANDLE_VALUE)
        return NULL;
    strcpy (dd->entry.d_name, dd->find_data.cFileName);
    if (!FindNextFile(dd->handle, &dd->find_data))
    {
        FindClose (dd->handle);
        dd->handle = INVALID_HANDLE_VALUE;
    }
    return &dd->entry;
}

void closedir(DIR *dd)
{
    if (dd->handle != INVALID_HANDLE_VALUE)
        FindClose (dd->handle);
    if (dd)
        free (dd);
}

#endif

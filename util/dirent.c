/*
 * Copyright (C) 1997-2002, Index Data
 * All rights reserved.
 *
 * $Id: dirent.c,v 1.5 2002-04-04 20:50:37 adam Exp $
 *
 * This utility implements a opendir/readdir/close on Windows.
 */

#include <ctype.h>
#include <assert.h>
#ifdef WIN32
#include <io.h>
#endif
#include <string.h>
#include <stdlib.h>

#include <direntz.h>

#ifdef WIN32

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


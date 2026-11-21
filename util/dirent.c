/* This file is part of the Zebra server.
   Copyright (C) Index Data

Zebra is free software; you can redistribute it and/or modify it under
the terms of the GNU General Public License as published by the Free
Software Foundation; either version 2, or (at your option) any later
version.

Zebra is distributed in the hope that it will be useful, but WITHOUT ANY
WARRANTY; without even the implied warranty of MERCHANTABILITY or
FITNESS FOR A PARTICULAR PURPOSE.  See the GNU General Public License
for more details.

You should have received a copy of the GNU General Public License
along with this program; if not, write to the Free Software
Foundation, Inc., 51 Franklin St, Fifth Floor, Boston, MA  02110-1301  USA

*/



#if HAVE_CONFIG_H
#include <config.h>
#endif
#include <ctype.h>
#include <assert.h>
#ifdef _WIN32
#include <errno.h>
#include <io.h>
#endif
#include <string.h>
#include <stdlib.h>

#include <direntz.h>

#ifdef _WIN32

struct DIR {
    HANDLE handle;
    WIN32_FIND_DATA find_data;
    struct dirent entry;
};

DIR *opendir (const char *name)
{
    char fullName[MAX_PATH+1];
    if (strlen(name) > sizeof(fullName) - sizeof("\\*.*"))
    {
        errno = ENAMETOOLONG;
        return NULL;
    }

    DIR *dd = malloc (sizeof(*dd));

    if (!dd)
        return NULL;
    strcpy (fullName, name);
    strcat (fullName, "\\*.*");
    dd->handle = FindFirstFile(fullName, &dd->find_data);
    if (dd->handle == INVALID_HANDLE_VALUE)
    {
        DWORD error = GetLastError();
        if (error == ERROR_FILE_NOT_FOUND)
        {
            /* An existing directory may have no matching entries. */
            DWORD attrs = GetFileAttributes(name);
            if (attrs != INVALID_FILE_ATTRIBUTES &&
                (attrs & FILE_ATTRIBUTE_DIRECTORY))
                return dd;
        }
        free(dd);
        if (error == ERROR_FILE_NOT_FOUND || error == ERROR_PATH_NOT_FOUND)
            errno = ENOENT;
        else if (error == ERROR_ACCESS_DENIED)
            errno = EACCES;
        else
            errno = EIO;
        return NULL;
    }
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

/*
 * Local variables:
 * c-basic-offset: 4
 * c-file-style: "Stroustrup"
 * indent-tabs-mode: nil
 * End:
 * vim: shiftwidth=4 tabstop=8 expandtab
 */


/*
 * Copyright (C) 1994-1995, Index Data I/S 
 * All rights reserved.
 * Sebastian Hammer, Adam Dickmeiss
 *
 * $Log: lockutil.c,v $
 * Revision 1.1  1995-12-07 17:38:47  adam
 * Work locking mechanisms for concurrent updates/commit.
 *
 */
#include <stdio.h>
#include <assert.h>
#include <unistd.h>
#include <sys/file.h>
#include <fcntl.h>
#include <string.h>
#include <errno.h>

#include <alexutil.h>
#include "index.h"

static char *lockDir = NULL;

void zebraLockPrefix (char *pathPrefix)
{
    if (!lockDir)
        lockDir = res_get_def (common_resource, "lockDir", "");
    assert (lockDir);
    
    strcpy (pathPrefix, lockDir);
    if (*pathPrefix && pathPrefix[strlen(pathPrefix)-1] != '/')
        strcat (pathPrefix, "/");
}

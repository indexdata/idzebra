/*
 * Copyright (C) 1994-1995, Index Data I/S 
 * All rights reserved.
 * Sebastian Hammer, Adam Dickmeiss
 *
 * $Log: lockutil.c,v $
 * Revision 1.2  1995-12-11 11:43:29  adam
 * Locking based on fcntl instead of flock.
 * Setting commitEnable removed. Command line option -n can be used to
 * prevent commit if commit setting is defined in the configuration file.
 *
 * Revision 1.1  1995/12/07  17:38:47  adam
 * Work locking mechanisms for concurrent updates/commit.
 *
 */
#include <stdio.h>
#include <assert.h>
#include <string.h>
#include <errno.h>
#include <fcntl.h>
#include <sys/types.h>
#include <unistd.h>

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

static int intLock (int fd, int type, int cmd)
{
    struct flock area;
    area.l_type = type;
    area.l_whence = SEEK_SET;
    area.l_len = area.l_start = 0L;
    return fcntl (fd, cmd, &area);
}

int zebraLock (int fd, int wr)
{
    return intLock (fd, wr ? F_WRLCK : F_RDLCK, F_SETLKW);
}

int zebraLockNB (int fd, int wr)
{
    return intLock (fd, wr ? F_WRLCK : F_RDLCK, F_SETLK);
}

int zebraUnlock (int fd)
{
    return intLock (fd, F_UNLCK, F_SETLKW);
}

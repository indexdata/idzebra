/*
 * Copyright (C) 1994-1995, Index Data I/S 
 * All rights reserved.
 * Sebastian Hammer, Adam Dickmeiss
 *
 * $Log: lockutil.c,v $
 * Revision 1.7  1997-09-09 13:38:08  adam
 * Partial port to WIN95/NT.
 *
 * Revision 1.6  1996/10/29 14:08:14  adam
 * Uses resource lockDir instead of lockPath.
 *
 * Revision 1.5  1996/03/26 16:01:13  adam
 * New setting lockPath: directory of various lock files.
 *
 * Revision 1.4  1995/12/13  08:46:10  adam
 * Locking uses F_WRLCK and F_RDLCK again!
 *
 * Revision 1.3  1995/12/12  16:00:57  adam
 * System call sync(2) used after update/commit.
 * Locking (based on fcntl) uses F_EXLCK and F_SHLCK instead of F_WRLCK
 * and F_RDLCK.
 *
 * Revision 1.2  1995/12/11  11:43:29  adam
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
#ifdef WINDOWS
#include <io.h>
#else
#include <unistd.h>
#endif

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
#if 0
    return intLock (fd, wr ? F_EXLCK : F_SHLCK, F_SETLKW);
#else
    return intLock (fd, wr ? F_WRLCK : F_RDLCK, F_SETLKW);
#endif
}

int zebraLockNB (int fd, int wr)
{
#if 0
    return intLock (fd, wr ? F_EXLCK : F_SHLCK, F_SETLK);
#else
    return intLock (fd, wr ? F_WRLCK : F_RDLCK, F_SETLK);
#endif
}

int zebraUnlock (int fd)
{
    return intLock (fd, F_UNLCK, F_SETLKW);
}

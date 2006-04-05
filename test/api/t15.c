/* $Id: t15.c,v 1.2 2006-04-05 02:03:33 adam Exp $
   Copyright (C) 2004-2005
   Index Data ApS

This file is part of the Zebra server.

Zebra is free software; you can redistribute it and/or modify it under
the terms of the GNU General Public License as published by the Free
Software Foundation; either version 2, or (at your option) any later
version.

Zebra is distributed in the hope that it will be useful, but WITHOUT ANY
WARRANTY; without even the implied warranty of MERCHANTABILITY or
FITNESS FOR A PARTICULAR PURPOSE.  See the GNU General Public License
for more details.

You should have received a copy of the GNU General Public License
along with Zebra; see the file LICENSE.zebra.  If not, write to the
Free Software Foundation, 59 Temple Place - Suite 330, Boston, MA
02111-1307, USA.
*/

#include <sys/types.h>
#if HAVE_UNISTD_H
#include <unistd.h>
#endif
#if HAVE_SYS_WAIT_H
#include <sys/wait.h>
#endif

#include "testlib.h"

static void update_process(ZebraService zs, int iter)
{
    int i;
    for (i = 0; i<iter; i++)
    {
	const char *rec = "<gils><title>some</title></gils>";
	ZebraHandle zh = zebra_open(zs, 0);
	// printf("update_record i=%d\n", i);
	zebra_add_record(zh, rec, strlen(rec));
	if ((i % 30) == 0 || i == iter-1)
	    zebra_commit(zh);
	zebra_close(zh);
    }
}

static void search_process(ZebraService zs, int iter)
{
    zint hits_max = 0;
    int i;
    for (i = 0; i<iter; i++)
    {
	ZebraHandle zh = zebra_open(zs, 0);
	zint hits;	    
	ZEBRA_RES res = zebra_search_PQF(zh, "@attr 1=4 some", "default",
					 &hits);
	YAZ_CHECK(res == ZEBRA_OK);
	
	YAZ_CHECK(hits >= hits_max);
	if (hits < hits_max)
	    printf("hits=%lld hits_max=%lld\n", hits, hits_max);
	    hits_max = hits;
	zebra_close(zh);
    }
}

static pid_t fork_service(ZebraService zs, int iter,
			   void (*f)(ZebraService zs, int iter))
{
    pid_t pid = fork();

    YAZ_CHECK(pid != -1);
    if (pid)
	return pid;
    
    (*f)(zs, iter);
    YAZ_CHECK_TERM;
}

static void tst(int argc, char **argv)
{
    ZebraService zs = tl_start_up("zebra15.cfg", argc, argv);

    ZebraHandle zh = zebra_open(zs, 0);

    YAZ_CHECK(zs);

    YAZ_CHECK(zh);

    YAZ_CHECK(zebra_select_database(zh, "Default") == ZEBRA_OK);

    zebra_init(zh);

    YAZ_CHECK(zebra_create_database (zh, "Default") == ZEBRA_OK);
    YAZ_CHECK(zebra_select_database(zh, "Default") == ZEBRA_OK);
    zebra_close(zh);

    update_process(zs, 1);

#if HAVE_SYS_WAIT_H
#if HAVE_UNISTD_H
    if (1)
    {
	int status[3];
	pid_t pids[3];

	pids[0] = fork_service(zs, 200, search_process);
	pids[1] = fork_service(zs, 20, update_process);
	pids[2] = fork_service(zs, 20, update_process);
	waitpid(pids[0], &status[0], 0);
	YAZ_CHECK(status[0] == 0);
	waitpid(pids[1], &status[1], 0);
	YAZ_CHECK(status[1] == 0);
	waitpid(pids[2], &status[2], 0);
	YAZ_CHECK(status[2] == 0);
    }
#endif
#endif

}

TL_MAIN

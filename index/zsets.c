/*
 * Copyright (C) 1994-1995, Index Data I/S 
 * All rights reserved.
 * Sebastian Hammer, Adam Dickmeiss
 *
 * $Log: zsets.c,v $
 * Revision 1.12  1997-09-25 14:57:36  adam
 * Windows NT port.
 *
 * Revision 1.11  1996/12/23 15:30:46  adam
 * Work on truncation.
 * Bug fix: result sets weren't deleted after server shut down.
 *
 * Revision 1.10  1995/10/30 15:08:08  adam
 * Bug fixes.
 *
 * Revision 1.9  1995/10/17  18:02:14  adam
 * New feature: databases. Implemented as prefix to words in dictionary.
 *
 * Revision 1.8  1995/10/10  13:59:25  adam
 * Function rset_open changed its wflag parameter to general flags.
 *
 * Revision 1.7  1995/10/06  14:38:01  adam
 * New result set method: r_score.
 * Local no (sysno) and score is transferred to retrieveCtrl.
 *
 * Revision 1.6  1995/09/28  09:19:49  adam
 * xfree/xmalloc used everywhere.
 * Extract/retrieve method seems to work for text records.
 *
 * Revision 1.5  1995/09/27  16:17:32  adam
 * More work on retrieve.
 *
 * Revision 1.4  1995/09/07  13:58:36  adam
 * New parameter: result-set file descriptor (RSFD) to support multiple
 * positions within the same result-set.
 * Boolean operators: and, or, not implemented.
 * Result-set references.
 *
 * Revision 1.3  1995/09/06  16:11:19  adam
 * Option: only one word key per file.
 *
 * Revision 1.2  1995/09/06  10:33:04  adam
 * More work on present. Some log messages removed.
 *
 * Revision 1.1  1995/09/05  15:28:40  adam
 * More work on search engine.
 *
 */
#include <stdio.h>
#include <assert.h>
#ifdef WINDOWS
#include <io.h>
#else
#include <unistd.h>
#endif

#include "zserver.h"
#include <rstemp.h>

ZServerSet *resultSetAdd (ZServerInfo *zi, const char *name, int ov, RSET rset)
{
    ZServerSet *s;

    for (s = zi->sets; s; s = s->next)
        if (!strcmp (s->name, name))
        {
            if (!ov)
                return NULL;
            rset_delete (s->rset);
            s->rset = rset;
            return s;
        }
    s = xmalloc (sizeof(*s));
    s->next = zi->sets;
    zi->sets = s;
    s->name = xmalloc (strlen(name)+1);
    strcpy (s->name, name);
    s->rset = rset;
    return s;
}

ZServerSet *resultSetGet (ZServerInfo *zi, const char *name)
{
    ZServerSet *s;

    for (s = zi->sets; s; s = s->next)
        if (!strcmp (s->name, name))
            return s;
    return NULL;
}

void resultSetDestroy (ZServerInfo *zi)
{
    ZServerSet *s, *s1;

    for (s = zi->sets; s; s = s1)
    {
        s1 = s->next;
        rset_delete (s->rset);
        xfree (s->name);
        xfree (s);
    }
    zi->sets = NULL;
}

ZServerSetSysno *resultSetSysnoGet (ZServerInfo *zi, const char *name, 
                                    int num, int *positions)
{
    ZServerSet *sset;
    ZServerSetSysno *sr;
    RSET rset;
    int num_i = 0;
    int position = 0;
    int psysno = 0;
    struct it_key key;
    RSFD rfd;

    if (!(sset = resultSetGet (zi, name)))
        return NULL;
    if (!(rset = sset->rset))
        return NULL;
    logf (LOG_DEBUG, "resultSetRecordGet");
    sr = xmalloc (sizeof(*sr) * num);
    rfd = rset_open (rset, RSETF_READ|RSETF_SORT_RANK);
    while (rset_read (rset, rfd, &key))
    {
        if (key.sysno != psysno)
        {
            psysno = key.sysno;
            position++;
            assert (num_i < num);
            if (position == positions[num_i])
            {
                sr[num_i].sysno = psysno;
                rset_score (rset, rfd, &sr[num_i].score);
                if (++num_i == num)
                    break;
            }
        }
    }
    rset_close (rset, rfd);
    while (num_i < num)
    {
        sr[num_i].sysno = 0;
        num_i++;
    }
    return sr;
}

void resultSetSysnoDel (ZServerInfo *zi, ZServerSetSysno *records, int num)
{
    xfree (records);
}

/*
 * Copyright (C) 1994-1998, Index Data I/S 
 * All rights reserved.
 * Sebastian Hammer, Adam Dickmeiss
 *
 * $Log: rset.h,v $
 * Revision 1.14  1998-02-10 11:56:46  adam
 * Implemented rset_dup.
 *
 * Revision 1.13  1997/12/18 10:54:24  adam
 * New method result set method rs_hits that returns the number of
 * hits in result-set (if known). The ranked result set returns real
 * number of hits but only when not combined with other operands.
 *
 * Revision 1.12  1997/09/05 15:30:03  adam
 * Changed prototype for chr_map_input - added const.
 * Added support for C++, headers uses extern "C" for public definitions.
 *
 * Revision 1.11  1995/12/11 09:07:53  adam
 * New rset member 'flag', that holds various flags about a result set -
 * currently 'volatile' (set is register dependent) and 'ranked' (set is
 * ranked).
 * New set types sand/sor/snot. They handle and/or/not for ranked and
 * semi-ranked result sets.
 *
 * Revision 1.10  1995/10/12  12:40:36  adam
 * Private info (buf) moved from struct rset_control to struct rset.
 * Member control in rset is statically set in rset_create.
 *
 * Revision 1.9  1995/10/10  14:00:01  adam
 * Function rset_open changed its wflag parameter to general flags.
 *
 * Revision 1.8  1995/10/06  14:37:53  adam
 * New result set method: r_score.
 * Local no (sysno) and score is transferred to retrieveCtrl.
 *
 * Revision 1.7  1995/09/07  13:58:08  adam
 * New parameter: result-set file descriptor (RSFD) to support multiple
 * positions within the same result-set.
 * Boolean operators: and, or, not implemented.
 *
 * Revision 1.6  1995/09/06  16:10:58  adam
 * More work on boolean sets.
 *
 * Revision 1.5  1995/09/04  15:20:13  adam
 * More work on temp sets. is_open member removed.
 *
 * Revision 1.4  1995/09/04  09:09:52  adam
 * String arg in dict lookup is const.
 * Minor changes.
 *
 * Revision 1.3  1994/11/22  13:15:27  quinn
 * Simple
 *
 * Revision 1.2  1994/11/04  13:21:21  quinn
 * Working.
 *
 * Revision 1.1  1994/11/03  14:13:22  quinn
 * Result set manipulation
 *
 */

#ifndef RSET_H
#define RSET_H

#include <stdlib.h>

#ifdef __cplusplus
extern "C" {
#endif
typedef void *RSFD;

typedef struct rset *RSET;

typedef struct rset_control
{
    char *desc; /* text description of set type (for debugging) */
    void *(*f_create)(const struct rset_control *sel, void *parms,
                      int *flags);
    RSFD (*f_open)(RSET ct, int wflag);
    void (*f_close)(RSFD rfd);
    void (*f_delete)(RSET ct);
    void (*f_rewind)(RSFD rfd);
    int (*f_count)(RSET ct);
    int (*f_hits)(RSET ct, void *oi);
    int (*f_read)(RSFD rfd, void *buf);
    int (*f_write)(RSFD rfd, const void *buf);
    int (*f_score)(RSFD rfd, int *score);
} rset_control;

typedef struct rset
{
    const rset_control *control;
    int  flags;
    int  count;
    void *buf;
} rset;

#define RSETF_READ       0
#define RSETF_WRITE      1

#define RSETF_SORT_SYSNO 0
#define RSETF_SORT_RANK  2

RSET rset_create(const rset_control *sel, void *parms);       /* parameters? */

/* int rset_open(RSET rs, int wflag); */
#define rset_open(rs, wflag) (*(rs)->control->f_open)((rs), (wflag))

/* void rset_close(RSET rs); */
#define rset_close(rs, rfd) (*(rs)->control->f_close)(rfd)

void rset_delete(RSET rs);

RSET rset_dup (RSET rs);

/* void rset_rewind(RSET rs); */
#define rset_rewind(rs, rfd) (*(rs)->control->f_rewind)((rfd))

/* int rset_count(RSET rs); */
#define rset_count(rs) (*(rs)->control->f_count)(rs)

/* int rset_hits (RSET) */
#define rset_hits(rs) (*(rs)->control->f_hits)((rs), 0)

/* int rset_read(RSET rs, void *buf); */
#define rset_read(rs, fd, buf) (*(rs)->control->f_read)((fd), (buf))

/* int rset_write(RSET rs, const void *buf); */
#define rset_write(rs, fd, buf) (*(rs)->control->f_write)((fd), (buf))

/* int rset_score(RSET rs, int *buf); */
#define rset_score(rs, fd, score) (*(rs)->control->f_score)((fd), (score))


/* int rset_type (RSET) */
#define rset_type(rs) ((rs)->control->desc)

#define RSET_FLAG_VOLATILE 1
#define RSET_FLAG_RANKED 2

#define rset_is_volatile(rs) ((rs)->flags & RSET_FLAG_VOLATILE)
#define rset_is_ranked(rs) ((rs)->flags & RSET_FLAG_RANKED)

#ifdef __cplusplus
}
#endif

#endif

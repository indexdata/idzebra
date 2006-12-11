/* $Id: timing.c,v 1.1 2006-12-11 09:50:36 adam Exp $
   Copyright (C) 1995-2006
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
along with this program; if not, write to the Free Software
Foundation, Inc., 51 Franklin St, Fifth Floor, Boston, MA  02110-1301  USA

*/

#include <stdlib.h>

#if HAVE_SYS_TIMES_H
#include <sys/times.h>
#endif
#if HAVE_SYS_TIME_H
#include <sys/time.h>
#endif

#include <yaz/xmalloc.h>
#include <idzebra/util.h>

struct zebra_timing {
    struct tms tms1, tms2;
    struct timeval start_time, end_time;
    double real_sec, user_sec, sys_sec;
};

zebra_timing_t zebra_timing_create(void)
{
    zebra_timing_t t = xmalloc(sizeof(*t));
    zebra_timing_start(t);
    return t;
}

void zebra_timing_start(zebra_timing_t t)
{
#if HAVE_SYS_TIMES_H
#if HAVE_SYS_TIME_H
    times(&t->tms1);
    gettimeofday(&t->start_time, 0);
#endif
#endif
    t->real_sec = -1.0;
    t->user_sec = -1.0;
    t->sys_sec = -1.0;
}

void zebra_timing_stop(zebra_timing_t t)
{
    t->real_sec = 0.0;
    t->user_sec = 0.0;
    t->sys_sec = 0.0;
#if HAVE_SYS_TIMES_H
#if HAVE_SYS_TIME_H
    gettimeofday(&t->end_time, 0);
    times(&t->tms2);
    
    t->real_sec = ((t->end_time.tv_sec - t->start_time.tv_sec) * 1000000.0 +
                   t->end_time.tv_usec - t->start_time.tv_usec) / 1000000;
    
    t->user_sec = (double) (t->tms2.tms_utime - t->tms1.tms_utime)/100;
    t->sys_sec = (double) (t->tms2.tms_stime - t->tms1.tms_stime)/100;
#endif
#endif
}

double zebra_timing_get_real(zebra_timing_t t)
{
    return t->real_sec;
}

double zebra_timing_get_user(zebra_timing_t t)
{
    return t->user_sec;
}

double zebra_timing_get_sys(zebra_timing_t t)
{
    return t->sys_sec;
}

void zebra_timing_destroy(zebra_timing_t *tp)
{
    if (*tp)
    {
        xfree(*tp);
        *tp = 0;
    }
}

/*
 * Local variables:
 * c-basic-offset: 4
 * indent-tabs-mode: nil
 * End:
 * vim: shiftwidth=4 tabstop=8 expandtab
 */


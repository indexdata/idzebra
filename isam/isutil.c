/*
 * Copyright (C) 1994, Index Data I/S 
 * All rights reserved.
 * Sebastian Hammer, Adam Dickmeiss
 *
 * $Log: isutil.c,v $
 * Revision 1.1  1994-09-12 08:02:13  quinn
 * Not functional yet
 *
 */

/*
 * Small utilities needed by the isam system. Some or all of these
 * may move to util/ along the way.
 */

#include <string.h>
#include <stdarg.h>

#include <isam.h>
#include "isutil.h"

char *strconcat(const char *s1, ...)
{
    va_list ap;
    static char buf[512];
    char *p;

    va_start(ap, s1);
    strcpy(buf, s1);
    while ((p = va_arg(ap, char *)))
    	strcat(buf, p);
    va_end(ap);
    
    return buf;
}

int is_default_cmp(const void *k1, const void *k2)
{
    SYSNO b1, b2;

    memcpy(&b1, k1, sizeof(b1));
    memcpy(&b2, k2, sizeof(b2));
    return b1 - b2;
}

/*
 * Copyright (C) 1994, Index Data I/S 
 * All rights reserved.
 * Sebastian Hammer, Adam Dickmeiss
 *
 * $Log: isutil.h,v $
 * Revision 1.1  1994-09-12 08:02:14  quinn
 * Not functional yet
 *
 */

/*
 * Small utilities needed by the isam system. Some or all of these
 * may move to util/ along the way.
 */

#ifndef ISUTIL_H
#define ISUTIL_H

char *strconcat(const char *s1, ...);

int is_default_cmp(const void *k1, const void *k2); /* compare function */

#endif

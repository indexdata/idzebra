/*
 * Copyright (C) 1994, Index Data I/S 
 * All rights reserved.
 * Sebastian Hammer, Adam Dickmeiss
 *
 * $Log: str.h,v $
 * Revision 1.2  1994-10-20 17:36:06  quinn
 * Minimal
 *
 * Revision 1.1  1994/10/20  13:46:36  quinn
 * String-management system
 *
 */

#ifndef STR_H
#define STR_H

typedef struct str_index
{
    int size;
    char *data;
} str_index;

typedef struct strings_data
{
    str_index *index;
    int num_index;
    char *bulk;
} strings_data, *STRINGS;

STRINGS str_open(char *lang, char *name);
void str_close(STRINGS st);
char *str(STRINGS st, int num);
char *strf(STRINGS st, int num, ...);

#endif

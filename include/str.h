/*
 * Copyright (C) 1994, Index Data I/S 
 * All rights reserved.
 * Sebastian Hammer, Adam Dickmeiss
 *
 * $Log: str.h,v $
 * Revision 1.3  1997-09-05 15:30:06  adam
 * Changed prototype for chr_map_input - added const.
 * Added support for C++, headers uses extern "C" for public definitions.
 *
 * Revision 1.2  1994/10/20 17:36:06  quinn
 * Minimal
 *
 * Revision 1.1  1994/10/20  13:46:36  quinn
 * String-management system
 *
 */

#ifndef STR_H
#define STR_H

#ifdef __cplusplus
extern "C" {
#endif

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

#ifdef __cplusplus
}
#endif

#endif

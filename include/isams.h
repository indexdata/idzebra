/*
 * Copyright (c) 1995-1999, Index Data.
 * See the file LICENSE for details.
 * Sebastian Hammer, Adam Dickmeiss
 *
 * $Id: isams.h,v 1.2 1999-07-14 10:59:26 adam Exp $
 */
#ifndef ISAMS_H
#define ISAMS_H

#include <bfile.h>

#ifdef __cplusplus
extern "C" {
#endif

typedef struct ISAMS_s *ISAMS;
typedef int ISAMS_P;
typedef struct ISAMS_PP_s *ISAMS_PP;

typedef struct ISAMS_M_s {
    int (*compare_item)(const void *a, const void *b);

#define ISAMC_DECODE 0
#define ISAMC_ENCODE 1
    void *(*code_start)(int mode);
    void (*code_stop)(int mode, void *p);
    void (*code_item)(int mode, void *p, char **dst, char **src);

    int debug;
    int block_size;
} *ISAMS_M;

typedef struct ISAMS_I_s {
    int (*read_item)(void *clientData, char **dst, int *insertMode);
    void *clientData;
} *ISAMS_I;

void isams_getmethod (ISAMS_M me);

ISAMS isams_open (BFiles bfs, const char *name, int writeflag,
		  ISAMS_M method);
int isams_close (ISAMS is);
ISAMS_P isams_merge (ISAMS is, ISAMS_I data);
ISAMS_PP isams_pp_open (ISAMS is, ISAMS_P pos);
void isams_pp_close (ISAMS_PP pp);
int isams_read_item (ISAMS_PP pp, char **dst);
int isams_pp_read (ISAMS_PP pp, void *buf);
int isams_pp_num (ISAMS_PP pp);

#ifdef __cplusplus
}
#endif

#endif

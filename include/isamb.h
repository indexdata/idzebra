/*
 * Copyright (C) 2000-2002, Index Data
 * All rights reserved.
 * $Id: isamb.h,v 1.4 2002-07-15 11:50:45 adam Exp $
 */

#ifndef ISAMB_H
#define ISAMB_H

#include <bfile.h>
#include <isamc.h>

typedef struct ISAMB_s *ISAMB;
typedef struct ISAMB_PP_s *ISAMB_PP;
typedef ISAMC_P ISAMB_P;

ISAMB isamb_open (BFiles bfs, const char *name, int writeflag, ISAMC_M method,
                  int cache);
void isamb_close (ISAMB isamb);

ISAMB_P isamb_merge (ISAMB b, ISAMB_P pos, ISAMC_I data);

ISAMB_PP isamb_pp_open (ISAMB isamb, ISAMB_P pos);

int isamb_pp_read (ISAMB_PP pp, void *buf);

void isamb_pp_close (ISAMB_PP pp);

int isamb_pp_num (ISAMB_PP pp);

ISAMB_PP isamb_pp_open_x (ISAMB isamb, ISAMB_P pos, int *level);
void isamb_pp_close_x (ISAMB_PP pp, int *size, int *blocks);

int isamb_block_info (ISAMB isamb, int cat);

#endif

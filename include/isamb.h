/*
 * $Id: isamb.h,v 1.1 2000-10-17 12:37:09 adam Exp $
 */

#ifndef ISAMB_H
#define ISAMB_H

#include <bfile.h>
#include <isamc.h>

typedef struct ISAMB_s *ISAMB;

ISAMB isamb_open (BFiles bfs, const char *name, ISAMC_M method);
void isamb_close (ISAMB isamb);

#endif

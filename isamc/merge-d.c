/*
 * Copyright (c) 1996-1998, Index Data.
 * See the file LICENSE for details.
 * Heikki Levanto
 *
 * merge-d.c: merge routines for isamd
 *
 */

#include <stdlib.h>
#include <assert.h>
#include <string.h>
#include <stdio.h>
#include <log.h>
#include "isamd-p.h"


static char *hexdump(unsigned char *p, int len, char *buff) {
  static char localbuff[128];
  char bytebuff[8];
  if (!buff) buff=localbuff;
  *buff='\0';
  while (len--) {
    sprintf(bytebuff,"%02x",*p);
    p++;
    strcat(buff,bytebuff);
    if (len) strcat(buff,",");
  }
  return buff;
}


/* isamd - heikki's append-only isam 
*/



ISAMD_P isamd_append (ISAMD is, ISAMD_P ipos, ISAMD_I data)
{
    
} /*  isamh_append */


/*
 * $Log: merge-d.c,v $
 * Revision 1.1  1999-07-14 13:14:47  heikki
 * Created empty
 *
 *
 */



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
 *
 * The record pointers arre kept in a linked list of buffers, as usual.
 * When modifying the list, we just store diffs at the end of it (either
 * in the only block, or in a separate block). Occasionally we have too
 * many blocks, and then we merge the diffs into the list.
 */



ISAMD_P isamd_append (ISAMD is, ISAMD_P ipos, ISAMD_I data)
{
  char i_item[128];    /* one input item */
  char *i_ptr=i_item;
  int i_more =1;
  int i_mode;     /* 0 for delete, 1 for insert */ 

  int retval=0;
  
  while (i_more)
  {
     i_ptr = i_item;
     i_more = (*data->read_item)(data->clientData, &i_ptr, &i_mode); 
     if (!retval)
       memcpy(&retval,i_item, sizeof(retval));
  }
  return retval;
} /*  isamh_append */


/*
 * $Log: merge-d.c,v $
 * Revision 1.2  1999-07-14 15:05:30  heikki
 * slow start on isam-d
 *
 * Revision 1.1  1999/07/14 13:14:47  heikki
 * Created empty
 *
 *
 */



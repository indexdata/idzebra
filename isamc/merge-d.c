/* $Id: merge-d.c,v 1.30.2.3 2006-12-05 21:14:42 adam Exp $
   Copyright (C) 1995,1996,1997,1998,1999,2000,2001,2002
   Index Data Aps

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




#define NEW_ISAM_D 1  /* not yet ready to delete the old one! */

#include <stdlib.h>
#include <assert.h>
#include <string.h>
#include <stdio.h>
#include <yaz/log.h>
#include "../index/index.h"
#include "isamd-p.h"


struct ISAMD_DIFF_s {
  int diffidx;
  int maxidx;
  struct it_key key;
  void *decodeData;
  int mode;
  int difftype;  
};

#define DT_NONE 0 /* no diff, marks end of sequence */
#define DT_DIFF 1 /* ordinarry diff */
#define DT_MAIN 2 /* main data */
#define DT_INPU 3 /* input data to be merged */
#define DT_DONE 4 /* done with all input here */



/***************************************************************
 * Input preprocess filter
 ***************************************************************/


#define FILTER_NOTYET -1  /* no data read in yet, to be done */

struct ISAMD_FILTER_s {
  ISAMD_I data;          /* where the data comes from */
  ISAMD is;              /* for debug flags */
  struct it_key k1;      /* the next item to be returned */
  int           m1;      /* mode for k1 */
  int           r1;      /* result for read of k1, or NOTYET */
  struct it_key k2;      /* the one after that */
  int           m2;
  int           r2;
};

typedef struct ISAMD_FILTER_s *FILTER;


void filter_fill(FILTER F)
{
  while ( (F->r1 == FILTER_NOTYET) || (F->r2 == FILTER_NOTYET) )
  {
     if (F->r1==FILTER_NOTYET) 
     { /* move data forward in the filter */
        F->k1 = F->k2;
        F->m1 = F->m2;
        F->r1 = F->r2;
        if ( 0 != F->r1 ) /* not eof */
          F->r2 = FILTER_NOTYET; /* say we want more */
        if (F->is->method->debug > 9)  
          yaz_log(YLOG_LOG,"filt_fill: shift %d.%d m=%d r=%d",
             F->k1.sysno, 
             F->k1.seqno, 
             F->m1, F->r1);
     }
     if (F->r2==FILTER_NOTYET)
     { /* read new bottom value */
        char *k_ptr = (char*) &F->k2;
        F->r2 = (F->data->read_item)(F->data->clientData, &k_ptr, &F->m2); 
        if (F->is->method->debug > 9)
          yaz_log(YLOG_LOG,"filt_fill: read %d.%d m=%d r=%d",
             F->k2.sysno, F->k2.seqno, F->m2, F->r2);
     }  
     if ( (F->k1.sysno == F->k2.sysno) && 
          (F->k1.seqno == F->k2.seqno) &&
          (F->m1 != F->m2) &&
          (F->r1 >0 ) && (F->r2 >0) )
     { /* del-ins pair of same key (not eof) , ignore both */
       if (F->is->method->debug > 9)
         yaz_log(YLOG_LOG,"filt_fill: skipped %d.%d m=%d/%d r=%d/%d",
            F->k1.sysno, F->k1.seqno, 
            F->m1,F->m2, F->r1,F->r2);
       F->r1 = FILTER_NOTYET;
       F->r2 = FILTER_NOTYET;
     }
  } /* while */
} /* filter_fill */


FILTER filter_open( ISAMD is, ISAMD_I data )
{
  FILTER F = (FILTER) xmalloc(sizeof(struct ISAMD_FILTER_s));
  F->is = is;
  F->data = data;
  F->k1.sysno=0;
  F->k1.seqno=0;
  F->k2=F->k1; 
  F->m1 = F->m2 = 0;
  F->r1 = F->r2 = FILTER_NOTYET;
  filter_fill(F);
  return F;
}

static void filter_close (FILTER F)
{
  xfree(F);
}

static int filter_read( FILTER F, 
                        struct it_key *k,
                        int *mode)
{
  int res;
  filter_fill(F);
  if (F->is->method->debug > 9)
    yaz_log(YLOG_LOG,"filt_read: reading %d.%d m=%d r=%d",
       F->k1.sysno, F->k1.seqno, F->m1, F->r1);
  res  = F->r1;
  if(res) 
  {
    *k = F->k1;
    *mode= F->m1;
  }
  F->r1 = FILTER_NOTYET;
  return res;
}

static int filter_isempty(FILTER F)
{
  return ( (0 == F->r1) && (0 == F->r2)) ;
}

#if 0
static int filter_only_one(FILTER F)
{
  return ( (0 != F->r1) && (0 == F->r2));
}
#endif

/* We may need backfilling, if we read a lonely key to make */
/* a singleton, but its bitw will not fit in. Then we need to */
/* process it normally, which means reading it again. So we  */
/* need to unread it first. Luckily the filter is empty at that */
/* point */
#if 0
static void filter_backfill(FILTER F, struct it_key *k, int mode)
{
  assert(F->r1 == FILTER_NOTYET ); /* not overwriting data! */
  F->k1=*k;
  F->m1=mode;
  F->r1=1; /* ok read */
}
#endif

/***************************************************************
 * Singleton encoding
 ***************************************************************/
/* When there is only a single item, we don't allocate a block
 * for it, but code it in the directory entry directly, if it
 * fits.
 */

#define DEC_SYSBITS 15
#define DEC_SEQBITS 15
#define DEC_MASK(n) ((1<<(n))-1)

#define SINGLETON_BIT (1<<(DEC_SYSBITS+DEC_SEQBITS+1))

int is_singleton(ISAMD_P ipos)
{
  return 0; /* no singletons any more */
  return ( ipos != 0 ) && ( ipos & SINGLETON_BIT );
}


int singleton_encode(struct it_key *k)
/* encodes the key into one int. If it does not fit, returns 0 */
{
  return 0; /* no more singletons */
  if ( (k->sysno & DEC_MASK(DEC_SYSBITS) ) != k->sysno )
    return 0;  /* no room dor sysno */
  if ( (k->seqno & DEC_MASK(DEC_SYSBITS) ) != k->seqno )
    return 0;  /* no room dor sysno */
  return (k->sysno | (k->seqno << DEC_SYSBITS) ) | SINGLETON_BIT;
}
 
void singleton_decode (int code, struct it_key *k)
{
  assert (code & SINGLETON_BIT);
  k->sysno = code & DEC_MASK(DEC_SYSBITS);
  code = code >> DEC_SYSBITS; 
  k->seqno = code & DEC_MASK(DEC_SEQBITS);
} 
 
 
/***************************************************************
 * General support routines
 ***************************************************************/



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



static void isamd_reduceblock(ISAMD_PP pp)
/* takes a large block, and reduces its category if possible */
/* Presumably the first block in an isam-list */
{
   if (pp->pos)
      return; /* existing block, do not touch */
      /* TODO: Probably we may touch anyway? */
   if (pp->is->method->debug > 5)  
     yaz_log(YLOG_LOG,"isamd_reduce: start p=%d c=%d sz=%d",
       pp->pos, pp->cat, pp->size); 
   while ( ( pp->cat > 0 ) && (!pp->next) && 
           (pp->offset < pp->is->method->filecat[pp->cat-1].bsize ) )
      pp->cat--;
   pp->pos = isamd_alloc_block(pp->is, pp->cat);
   if (pp->is->method->debug > 5) 
     yaz_log(YLOG_LOG,"isamd_reduce:  got  p=%d c=%d sz=%d",
       pp->pos, pp->cat, pp->size);    
} /* reduceblock */


static int save_first_pp ( ISAMD_PP firstpp)
{
   isamd_buildfirstblock(firstpp);
   isamd_write_block(firstpp->is,firstpp->cat,firstpp->pos,firstpp->buf);
   return isamd_addr(firstpp->pos,firstpp->cat);
}

 
static void save_last_pp (ISAMD_PP pp)
{
   pp->next = 0;/* just to be sure */
   isamd_buildlaterblock(pp);
   isamd_write_block(pp->is,pp->cat,pp->pos,pp->buf);
}

#ifdef UNUSED
static int save_both_pps (ISAMD_PP firstpp, ISAMD_PP pp)
{
   /* order of things: Better to save firstpp first, if there are just two */
   /* blocks, but last if there are blocks in between, as these have already */
   /* been saved... optimise later (that's why this is in its own func...*/
   int retval = save_first_pp(firstpp);
   if (firstpp!=pp){ 
      save_last_pp(pp);
      isamd_pp_close(pp);
   }
   isamd_pp_close(firstpp);
   return retval;
} /* save_both_pps */
#endif



/***************************************************************
 * Diffblock handling
 ***************************************************************/

void isamd_free_diffs(ISAMD_PP pp)
{
  int i;
  if (pp->is->method->debug > 5)
     yaz_log(YLOG_LOG,"isamd_free_diffs: pp=%p di=%p", pp, pp->diffinfo);
  if (!pp->diffinfo) 
    return;
  for (i=0;pp->diffinfo[i].difftype!=DT_NONE;i++) 
      if(pp->diffinfo[i].decodeData)
      {
          if (pp->is->method->debug > 8)
             yaz_log(YLOG_LOG,"isamd_free_diffs [%d]=%p",i, 
                           pp->diffinfo[i].decodeData);
          (*pp->is->method->code_stop)(ISAMD_DECODE,pp->diffinfo[i].decodeData);
      } 
  xfree(pp->diffinfo);
  if (pp->diffbuf != pp->buf)
    xfree (pp->diffbuf);  
  pp->diffbuf=0;
  pp->diffinfo=0;
} /* isamd_free_diffs */


static void getDiffInfo(ISAMD_PP pp )
{ /* builds the diff info structures from a diffblock */
   int maxinfos = pp->is->method->filecat[pp->cat].bsize / 5 +2;
    /* Each diff takes at least 5 bytes. Probably more, but this is safe */
   int i=1;  /* [0] is used for the main data, [n+1] for merge inputs */
   int diffsz= maxinfos * sizeof(struct ISAMD_DIFF_s);
   int maxsz = pp->is->method->filecat[pp->is->max_cat].bsize;
   int diffidx = ISAMD_BLOCK_OFFSET_1;

   pp->diffinfo = xmalloc( diffsz ); 
   pp->offset = pp->size+1; /* used this block up */
   memset(pp->diffinfo,'\0',diffsz);
   if (pp->is->method->debug > 5)   
      yaz_log(YLOG_LOG,"isamd_getDiffInfo: %d=%d:%d->%d, ix=%d mx=%d",
         isamd_addr(pp->pos, pp->cat), pp->cat, pp->pos, pp->next,
         diffidx,maxinfos);
   
   /* duplicate the buffer for diffs */
   /* (so that we can read the next real buffer(s) */
   assert(0==pp->diffbuf);
   pp->diffbuf=xmalloc(maxsz);
   memcpy(pp->diffbuf, pp->buf, maxsz);
   
   pp->diffinfo[0].maxidx=-1; /* mark as special */
   pp->diffinfo[0].difftype=DT_MAIN;

   while (i<maxinfos) 
   {  
      if ( diffidx+sizeof(int) > pp->is->method->filecat[pp->cat].bsize )
      {
         if (pp->is->method->debug > 5)
           yaz_log(YLOG_LOG,"isamd_getDiffInfo:Near end (no room for len) at ix=%d n=%d",
               diffidx, i);
         return; /* whole block done */
      }
      memcpy( &pp->diffinfo[i].maxidx, &pp->diffbuf[diffidx], sizeof(int) );
      pp->diffinfo[i].difftype=DT_DIFF;
      if (pp->is->method->debug > 5)
        yaz_log(YLOG_LOG,"isamd_getDiffInfo: max=%d ix=%d dbuf=%p",
          pp->diffinfo[i].maxidx, diffidx, pp->diffbuf);

      if ( (pp->is->method->debug > 0) &&
         (pp->diffinfo[i].maxidx > pp->is->method->filecat[pp->cat].bsize) )
      { 
         yaz_log(YLOG_LOG,"Bad MaxIx!!! %s:%d: diffidx=%d", 
                       __FILE__,__LINE__, diffidx);
         yaz_log(YLOG_LOG,"i=%d maxix=%d bsz=%d", i, pp->diffinfo[i].maxidx,
                       pp->is->method->filecat[pp->cat].bsize);
         yaz_log(YLOG_LOG,"pp=%d=%d:%d  pp->nx=%d=%d:%d",
                       isamd_addr(pp->pos,pp->cat), pp->pos, pp->cat,
                       pp->next, isamd_type(pp->next), isamd_block(pp->next) );                      
      }
      assert(pp->diffinfo[i].maxidx <= pp->is->method->filecat[pp->cat].bsize+1);

      if (0==pp->diffinfo[i].maxidx)
      {
         if (pp->is->method->debug > 5)  /* !!! 4 */
           yaz_log(YLOG_LOG,"isamd_getDiffInfo:End mark at ix=%d n=%d",
               diffidx, i);
         return; /* end marker */
      }
      diffidx += sizeof(int);
      pp->diffinfo[i].decodeData = (*pp->is->method->code_start)(ISAMD_DECODE);
      pp->diffinfo[i].diffidx = diffidx;
      if (pp->is->method->debug > 5)
        yaz_log(YLOG_LOG,"isamd_getDiff[%d]:%d-%d %s",
          i,diffidx-sizeof(int),pp->diffinfo[i].maxidx,
          hexdump((unsigned char *)&pp->diffbuf[diffidx-4],8,0) );
      diffidx=pp->diffinfo[i].maxidx;
      if ( diffidx > pp->is->method->filecat[pp->cat].bsize )
        return; /* whole block done */
      ++i;
   }
   assert (!"too many diff sequences in the block");
}

/***************************************************************
 * Main block operations
 ***************************************************************/


static ISAMD_PP get_new_main_block( ISAMD_PP firstpp, ISAMD_PP pp)
{  /* allocates a new block for the main data, and links it in */
  int newblock;
  if (0 == firstpp->next) 
  {  /* special case, pp not yet allocated. */
     /*Started as largest size, that's fine */
     pp->pos = isamd_alloc_block(pp->is,pp->cat);
     firstpp->next = isamd_addr(pp->pos,pp->cat);
     if (pp->is->method->debug >3) 
        yaz_log(YLOG_LOG,"isamd_build: Alloc 1. dblock  p=%d=%d:%d",
           isamd_addr(pp->pos,pp->cat), pp->cat, pp->pos);
  }
  newblock=isamd_alloc_block(pp->is,pp->cat);
  pp->next=isamd_addr(newblock,pp->cat);
  isamd_buildlaterblock(pp);
  isamd_write_block(pp->is,pp->cat,pp->pos,pp->buf);
  if (pp->is->method->debug >3) 
     yaz_log(YLOG_LOG,"isamd_build: Alloc nxt %d=%d:%d -> %d=%d:%d",
        isamd_addr(pp->pos,pp->cat), pp->cat, pp->pos,
        isamd_addr(newblock,pp->cat), pp->cat, newblock);
  pp->next=0;
  pp->pos=newblock;
  pp->size=pp->offset=ISAMD_BLOCK_OFFSET_N;  
  return pp;
} /* get_new_main_block */


static ISAMD_PP  append_main_item(ISAMD_PP firstpp, 
                                  ISAMD_PP pp, 
                                  struct it_key *i_key)
{  /* appends one item in the main data block, allocates new if needed */
   char *i_item= (char *) i_key;  /* same as char */
   char *i_ptr=i_item;
   char codebuff[128];
   char *c_ptr = codebuff;
   int codelen;
   char hexbuff[64];

   int maxsize = pp->is->method->filecat[pp->is->max_cat].bsize; 

   c_ptr=codebuff;
   i_ptr=i_item;
   (*pp->is->method->code_item)(ISAMD_ENCODE, pp->decodeClientData,
                                &c_ptr, &i_ptr);
   codelen = c_ptr - codebuff;
   assert ( (codelen<128) && (codelen>0));
   if (pp->is->method->debug >7)
      yaz_log(YLOG_LOG,"isamd:build: coded %s nk=%d,ofs=%d-%d",
          hexdump((unsigned char *) codebuff, c_ptr-codebuff,hexbuff), firstpp->numKeys+1,
          pp->offset, pp->offset+codelen);

   if (pp->offset + codelen > maxsize )
   { /* oops, block full - get a new one */
      pp =  get_new_main_block( firstpp, pp );
      /* reset encoging and code again */
      (*pp->is->method->code_reset)(pp->decodeClientData);
      c_ptr=codebuff;
      i_ptr=i_item;
      (*pp->is->method->code_item)(ISAMD_ENCODE, pp->decodeClientData, 
                                   &c_ptr, &i_ptr);
      codelen = c_ptr - codebuff;
      assert ( (codelen<128) && (codelen>0));
      if (pp->is->method->debug >7)
         yaz_log(YLOG_LOG,"isamd:build: recoded into %s  (nk=%d)",
             hexdump((unsigned char *) codebuff, c_ptr-codebuff,hexbuff), firstpp->numKeys+1);
   } /* block full */    

   assert (pp->offset + codelen <= maxsize );
   
   /* write the data into pp, now we must have room */ 
   memcpy(&(pp->buf[pp->offset]),codebuff,codelen);
   pp->offset += codelen;
   pp->size += codelen;
   firstpp->numKeys++;
   /* clear the next 4 bytes in block, to avoid confusions with diff lens */
   /* dirty, it should not be done here, but something slips somewhere, and */
   /* I hope this fixes it...  - Heikki */
   codelen = pp->offset;
   while ( (codelen < maxsize ) && (codelen <= pp->offset+4) )
     pp->buf[codelen++] = '\0';
   return pp;    
} /* append_main_item */


/***************************************************************
 * Read with merge
 ***************************************************************/

/* Reads one item and corrects for the diffs, if any */
/* return 1 for ok, 0 for eof */
int isamd_read_item_merge (
                     ISAMD_PP pp, 
                     char **dst,
                     struct it_key *p_key,  /* the data item that didn't fit*/
              /*       ISAMD_I data)  */    /* more input data comes here */
                     FILTER filt)           /* more input data comes here */
{                    /* The last two args can be null for ordinary reads */
  char *keyptr;
  char *codeptr;
  char *codestart;
  int winner=0; /* which diff holds the day */
  int i; /* looping diffs */
  int cmp;
  int retry=1;
  int oldoffs;
  int rc;

  if (!pp->diffinfo)  
  { /* first time */
     getDiffInfo(pp);

     for(i=1; pp->diffinfo[i].difftype!=DT_NONE; i++)
       ;  /* find last diff */
     if (p_key)  
     {  /* we have an extra item to inject into the merge */
       if (pp->is->method->debug >9)  /* !!!!! */
          yaz_log(YLOG_LOG,"isamd_read_item: going to merge with %d.%d",
               p_key->sysno, p_key->seqno);
       pp->diffinfo[i].key = *p_key;  /* the key merge could not handle */
       pp->diffinfo[i].mode = pp->diffinfo[i].key.seqno & 1;
       pp->diffinfo[i].key.seqno >>= 1;
       pp->diffinfo[i].difftype=DT_INPU;
       if (pp->is->method->debug > 7)
         yaz_log(YLOG_LOG,"isamd_read_item: inpu key %d sys=%d  seq=%d=2*%d+%d",
            i, p_key->sysno,
            pp->diffinfo[i].key.seqno*2 + pp->diffinfo[1].mode,
            pp->diffinfo[i].key.seqno,
            pp->diffinfo[i].mode);
       p_key->sysno=p_key->seqno=0;  /* used it up */
     }

     if (filt)
     { /* we have a whole input stream to inject */
       pp->diffinfo[i].difftype=DT_INPU;
     }
  } /* first time */ 
        
  while (retry)

  {
     retry=0;     
     winner = 0;
     for (i=0; (!retry) && (pp->diffinfo[i].difftype); i++)
     {
        if (0==pp->diffinfo[i].key.sysno)
        {/* read a new one, if possible */
           if ((pp->diffinfo[i].difftype==DT_DIFF) &&
             (pp->diffinfo[i].diffidx < pp->diffinfo[i].maxidx))
           { /* a normal kind of diff */
              oldoffs=pp->diffinfo[i].diffidx;
              codeptr= codestart = &(pp->diffbuf[pp->diffinfo[i].diffidx]);
              keyptr=(char *)&(pp->diffinfo[i].key);
              (*pp->is->method->code_item)(ISAMD_DECODE,
                    pp->diffinfo[i].decodeData, &keyptr, &codeptr);
              pp->diffinfo[i].diffidx += codeptr-codestart;
              pp->diffinfo[i].mode = pp->diffinfo[i].key.seqno & 1;
              pp->diffinfo[i].key.seqno = pp->diffinfo[i].key.seqno >>1 ;
              if (pp->is->method->debug > 9)
                yaz_log(YLOG_LOG,"isamd_read_item: dif[%d] at %d-%d: %s",
                  i,oldoffs, pp->diffinfo[i].diffidx,
                  hexdump((unsigned char *) pp->buf+oldoffs, pp->diffinfo[i].diffidx-oldoffs,0));
              if (pp->is->method->debug > 7)
                yaz_log(YLOG_LOG,"isamd_read_item: rd dif[%d] %d.%d (%x.%x)",
                  i,
                  pp->diffinfo[i].key.sysno, pp->diffinfo[i].key.seqno,
                  pp->diffinfo[i].key.sysno, pp->diffinfo[i].key.seqno);
           }
           else if ( pp->diffinfo[i].difftype==DT_MAIN)
           { /* read a main item */
              assert(i==0);  /* main data goes before any diffs */
              oldoffs=pp->offset;
              keyptr=(char*) &(pp->diffinfo[0].key);
              rc= isamd_read_main_item(pp,&keyptr);
              if (0==rc) 
              { /* eof */
                 if (pp->is->method->debug > 7)
                   yaz_log(YLOG_LOG,"isamd_read_item: eof (rc=%d) main ",
                           rc);
                pp->diffinfo[i].maxidx=-1;
                pp->diffinfo[i].key.sysno=0;
                pp->diffinfo[i].key.seqno=0;
                pp->diffinfo[i].difftype= DT_DONE;
              } 
              else
              { /* not eof */
                 pp->diffinfo[i].mode = 1;
                 if (pp->is->method->debug > 7)
                   yaz_log(YLOG_LOG,"isamd_read_item: rd main %d-%d %d.%d (%x.%x) m=%d",
                     oldoffs,pp->offset,
                     pp->diffinfo[i].key.sysno, pp->diffinfo[i].key.seqno,
                     pp->diffinfo[i].key.sysno, pp->diffinfo[i].key.seqno,
                     pp->diffinfo[i].mode);
              } /* not eof */
           }
           else if (pp->diffinfo[i].difftype==DT_INPU)
           {
              keyptr = (char *) &pp->diffinfo[i].key;
         /*     rc = (*data->read_item)(data->clientData, &keyptr, &pp->diffinfo[i].mode); */
              rc = filter_read(filt, &pp->diffinfo[i].key, 
                                     &pp->diffinfo[i].mode); 
              if (!rc) 
              {  /* did not get it */
                 pp->diffinfo[i].key.sysno=0;
                 pp->diffinfo[i].maxidx=0; /* signal the end */
                 pp->diffinfo[i].difftype=DT_DONE;
              }
              if (pp->is->method->debug >7)
                 yaz_log(YLOG_LOG,"merge: read inpu m=%d %d.%d (%x.%x)",
                    pp->diffinfo[i].mode, 
                    pp->diffinfo[i].key.sysno, pp->diffinfo[i].key.seqno,
                    pp->diffinfo[i].key.sysno, pp->diffinfo[i].key.seqno );        
           } /* read an input item */
        } /* read a new one */
        
        if (pp->is->method->debug > 8)
          yaz_log(YLOG_LOG,"isamd_read_item: considering d%d %d.%d ix=%d mx=%d",
               i, pp->diffinfo[i].key.sysno, pp->diffinfo[i].key.seqno,
                  pp->diffinfo[i].diffidx,   pp->diffinfo[i].maxidx);
            
        if ( 0!= pp->diffinfo[i].key.sysno)
        { /* got a key, compare */
          if (i!=winner)
             cmp=key_compare(&pp->diffinfo[i].key, &pp->diffinfo[winner].key);
          else
             cmp=-1;
          if (0==pp->diffinfo[winner].key.sysno)
            cmp=-1; /* end of main sequence, take all diffs */
          if (cmp<0)
          {
             if (pp->is->method->debug > 8)
               yaz_log(YLOG_LOG,"isamd_read_item: ins [%d]%d.%d < [%d]%d.%d",
                 i,  
                 pp->diffinfo[i].key.sysno, pp->diffinfo[i].key.seqno,
                 winner, 
                 pp->diffinfo[winner].key.sysno, pp->diffinfo[winner].key.seqno);
             if (pp->diffinfo[i].mode)  /* insert diff, should always be */
               winner = i;
             else
             {
               if (pp->is->method->debug > 1)
                 yaz_log(YLOG_LOG,"delete diff for nonexisting item");
               assert(!"delete diff for nonexisting item");  
               /* is an assert too steep here? Not really.*/
             }
          } /* earlier key */
          else if (cmp==0)
          {
             if (!pp->diffinfo[i].mode) /* delete diff. should always be */
             {
                if (pp->is->method->debug > 8)
                  yaz_log(YLOG_LOG,"isamd_read_item: del %d at%d %d.%d (%x.%x)",
                    i, winner,
                    pp->diffinfo[i].key.sysno, pp->diffinfo[i].key.seqno,
                    pp->diffinfo[i].key.sysno, pp->diffinfo[i].key.seqno);
                pp->diffinfo[winner].key.sysno=0; /* delete it */
             }
             else
                if (pp->is->method->debug > 2)
                  yaz_log(YLOG_LOG,"isamd_read_item: duplicate ins %d at%d %d.%d (%x.%x)",
                    i, winner,
                    pp->diffinfo[i].key.sysno, pp->diffinfo[i].key.seqno,
                    pp->diffinfo[i].key.sysno, pp->diffinfo[i].key.seqno);
                /* skip the insert, since we already have it in the base */
                /* Should we fail an assertion here??? */
             pp->diffinfo[i].key.sysno=0; /* done with the delete */
             retry=1; /* start all over again */
          } /* matching key */
          /* else it is a later key, its turn will come */
        } /* got a key */
     } /* for each diff */
  } /* not retry */

  if ( pp->diffinfo[winner].key.sysno)
  {
    if (pp->is->method->debug > 7)
      yaz_log(YLOG_LOG,"isamd_read_item: got %d  %d.%d (%x.%x)",
        winner,
        pp->diffinfo[winner].key.sysno, pp->diffinfo[winner].key.seqno,
        pp->diffinfo[winner].key.sysno, pp->diffinfo[winner].key.seqno);
    memcpy(*dst, &pp->diffinfo[winner].key, sizeof(struct it_key) );
    *dst += sizeof(struct it_key);
    pp->diffinfo[winner].key.sysno=0; /* used that one up */
    cmp= 1;
  } 
  else 
  {
    if (pp->is->method->debug > 7)
      yaz_log(YLOG_LOG,"isamd_read_item: eof w=%d  %d.%d (%x.%x)",
        winner,
        pp->diffinfo[winner].key.sysno, pp->diffinfo[winner].key.seqno,
        pp->diffinfo[winner].key.sysno, pp->diffinfo[winner].key.seqno);
    assert(winner==0); /* if nothing found, nothing comes from a diff */
    cmp= 0; /* eof */
  }
  if (cmp)
    ++(pp->is->no_read_keys);
  else
    ++(pp->is->no_read_eof);

  return cmp;
   
} /* isamd_read_item */


int isamd_read_item (ISAMD_PP pp, char **dst)
{
  return isamd_read_item_merge(pp,dst,0,0);
}


/***************************************************************
 * Merge
 ***************************************************************/

static int merge ( ISAMD_PP firstpp,      /* first pp (with diffs) */
                   struct it_key *p_key,  /* the data item that didn't fit*/
                   FILTER filt,           /* more input data arriving here */
                   char *dictentry,       /* the thin in the dictionary */
                   int dictlen)           /* and its size */
{
  int diffidx;  
  int killblk=0;
  struct it_key r_key;
  char * r_ptr;
  int r_more = 1;
  ISAMD_PP pp;
  ISAMD_PP readpp=firstpp;
  int retpos=0;
  int diffcat = firstpp->cat;  /* keep the category of the diffblock even */
                               /* if it is going to be empty now. */
                               /* Alternative: Make it the minimal, and */
                               /* resize later. Saves disk, but will lead */
                               /* into bad seeks. */
  
  ++(readpp->is->no_merges);
     
  /* set up diffs as they should be for reading */
  diffidx = ISAMD_BLOCK_OFFSET_1; 
  
  if (readpp->is->method->debug >4) 
      yaz_log(YLOG_LOG,"isamd_merge: f=%d=%d:%d n=%d=%d:%d",
        isamd_addr(firstpp->pos,firstpp->cat), firstpp->cat, firstpp->pos,
        firstpp->next, isamd_type(firstpp->next), isamd_block(firstpp->next));  

  /* release our data block. Do before reading, when pos is stable ! */
  killblk=firstpp->pos;
  if (killblk)
  {
      isamd_release_block(firstpp->is, firstpp->cat, killblk);
      if (readpp->is->method->debug >3)   
          yaz_log(YLOG_LOG,"isamd_merge: released old firstblock %d (%d:%d)",
              isamd_addr(killblk,firstpp->cat), firstpp->cat, killblk );
  }
  

  r_ptr= (char *) &r_key;
  r_more = isamd_read_item_merge( readpp, &r_ptr, p_key, filt);
  if (!r_more)  
  { /* oops, all data has been deleted! what to do??? */
    /* never mind, we have at least one more delta to add to the block */
    /* pray that is not a delete as well... */
    r_key.sysno = 0;
    r_key.seqno = 0;
     if (readpp->is->method->debug >5) 
         yaz_log(YLOG_LOG,"isamd_merge:all data has been deleted (nk=%d) ",
            readpp->numKeys);
  }


  /* set up the new blocks for simple writing */
  /* firstpp=isamd_pp_open(readpp->is,isamd_addr(0, diffcat)); */
  firstpp=isamd_pp_create(readpp->is, diffcat);
  firstpp->pos=isamd_alloc_block(firstpp->is,diffcat);
  if (readpp->is->method->debug >3)   
      yaz_log(YLOG_LOG,"isamd_merge: allocated new firstpp %d=%d:%d",
          isamd_addr(firstpp->pos,firstpp->cat), firstpp->cat, firstpp->pos );
  
  pp=isamd_pp_create(readpp->is,readpp->is->max_cat );
  pp->offset=pp->size=ISAMD_BLOCK_OFFSET_N;
  
  while (r_more)
  {
     if (readpp->is->method->debug >6) 
         yaz_log(YLOG_LOG,"isamd_merge: got key %d.%d",
           r_key.sysno, r_key.seqno );
     pp= append_main_item(firstpp, pp, &r_key);

     if ( (readpp->pos != killblk ) && (0!=readpp->pos) )
     {  /* pos can get to 0 at end of main seq, if still diffs left...*/
        if (readpp->is->method->debug >3)  
            yaz_log(YLOG_LOG,"isamd_merge: released block %d (%d:%d) now %d=%d:%d",
                isamd_addr(killblk,readpp->cat), readpp->cat, killblk,
                isamd_addr(readpp->pos,readpp->cat),readpp->cat, readpp->pos );
        isamd_release_block(readpp->is, readpp->cat, readpp->pos);
        killblk=readpp->pos;
     }

     /* (try to) read next item */
     r_ptr= (char *) &r_key;
     r_more = isamd_read_item_merge( readpp, &r_ptr,0,filt);

  } /* while read */
  
  
  isamd_reduceblock(pp);  /* reduce size if possible */
  if (0==firstpp->next)
    firstpp->next = isamd_addr(pp->pos,pp->cat);
  save_last_pp(pp);
  if (readpp->is->method->debug >4) 
      yaz_log(YLOG_LOG,"isamd_merge: saved last block %d=%d:%d",
            isamd_addr(pp->pos,pp->cat), pp->cat, pp->pos);
  isamd_pp_close(pp);

  if (readpp->is->method->debug >5) 
        yaz_log(YLOG_LOG,"isamd_merge: closing readpp %d=%d:%d di=%p",
              isamd_addr(readpp->pos,readpp->cat), readpp->cat, readpp->pos,
              readpp->diffinfo);
  isamd_pp_close(readpp); /* pos is 0 by now, at eof. close works anyway */

  if (readpp->is->method->debug >2)  
      yaz_log(YLOG_LOG,"isamd_merge: merge ret f=%d=%d:%d pp=%d=%d:%d",
            isamd_addr(firstpp->pos,pp->cat), firstpp->cat, firstpp->pos,
            isamd_addr(pp->pos,pp->cat), pp->cat, pp->pos);

  firstpp->size = firstpp->offset = ISAMD_BLOCK_OFFSET_1;  /* nothing there */
  memset(firstpp->buf,'\0',firstpp->is->method->filecat[firstpp->cat].bsize);
  save_first_pp(firstpp);
  retpos = isamd_addr(firstpp->pos, firstpp->cat);
  isamd_pp_close(firstpp);

  /* Create the dict entry */
  /*!*/ /* it could be this could go in the dict as well, if there's */
        /* been really many deletes. Somehow I suspect that is not the */
        /* case. FIXME: Collect statistics and see if needed */
  dictentry[0]=0; /* mark as a real isam */
  memcpy(dictentry+1, &retpos, sizeof(ISAMD_P));
  dictlen=sizeof(ISAMD_P)+1;
  return dictlen;
  
} /* merge */




/***************************************************************
 * Appending diffs 
 ***************************************************************/



static int append_diffs(
      ISAMD is, 
      char *dictentry, int dictlen,
      FILTER filt)
{
   ISAMD_P ipos;
   struct it_key i_key;    /* one input item */
   char *i_item = (char *) &i_key;  /* same as chars */
   char *i_ptr=i_item;
   int i_more =1;
   int i_mode = 1;     /* 0 for delete, 1 for insert */ 

   ISAMD_PP firstpp;
   char hexbuff[64];
   int diffidx=0;
   int maxsize=0;
   int difflenidx;
   char codebuff[128];
   char *c_ptr = codebuff;
   int codelen;
   int merge_rc;
   ISAMD_P retpos;
   int dsize;

   if (0==dictlen)
   {
       firstpp=isamd_pp_create(is, 0 );
       firstpp->size=firstpp->offset=ISAMD_BLOCK_OFFSET_1;
         /* create in smallest category, will expand later */
       ++(is->no_fbuilds);
   } 
   else
   {
       firstpp=isamd_pp_open(is, dictentry, dictlen);
       if (dictentry[0] )
          ipos=0;
       else 
           memcpy(&ipos,dictentry+1,sizeof(ISAMD_P));
       ++(is->no_appds);
   }

   if (is->method->debug >2) 
      yaz_log(YLOG_LOG,"isamd_appd: Start ipos=%d=%d:%d n=%d=%d:%d nk=%d sz=%d",
        ipos, isamd_type(ipos), isamd_block(ipos),
        firstpp->next, isamd_type(firstpp->next), isamd_block(firstpp->next),
        firstpp->numKeys, firstpp->size);
   maxsize = is->method->filecat[firstpp->cat].bsize; 
   
   difflenidx = diffidx = firstpp->size;
   
   diffidx+=sizeof(int);  /* difflen will be stored here */
   
   /* read first input */
   i_more = filter_read(filt, &i_key, &i_mode); 
   /* i_more = (*data->read_item)(data->clientData, &i_ptr, &i_mode); */

   if (is->method->debug >6)
      yaz_log(YLOG_LOG,"isamd_appd: start m=%d %d.%d=%x.%x: %d",
         i_mode, 
         i_key.sysno, i_key.seqno, 
         i_key.sysno, i_key.seqno,
         i_key.sysno*2+i_mode);

   while (i_more)
   {     
      /* store the mode bit inside key */
      assert( ((i_key.seqno<<1)>>1) == i_key.seqno); /* can spare the bit */
      i_key.seqno = i_key.seqno * 2 + i_mode;  

      c_ptr=codebuff;
      i_ptr=i_item; 
      (*is->method->code_item)(ISAMD_ENCODE, firstpp->decodeClientData, 
                               &c_ptr, &i_ptr);
      codelen = c_ptr - codebuff;
      assert ( (codelen<128) && (codelen>0));
      if (is->method->debug >7)
         yaz_log(YLOG_LOG,"isamd_appd: coded %d: %s (nk=%d) (ix=%d)",
             codelen, hexdump((unsigned char *) codebuff, codelen,hexbuff), 
             firstpp->numKeys,diffidx);

      if (diffidx + codelen > maxsize )
      { /* block full */
         while ( (firstpp->cat < firstpp->is->max_cat) &&
                 (diffidx + codelen > maxsize) )
         { /* try to increase the block size */
             if (firstpp->pos > 0)  /* free the old block if allocated */
                 isamd_release_block(is, firstpp->cat, firstpp->pos);
             ++firstpp->cat;
             maxsize = is->method->filecat[firstpp->cat].bsize; 
             firstpp->pos=0; /* need to allocate it when saving */             
             if (is->method->debug >3)
                yaz_log(YLOG_LOG,"isamd_appd: increased diff block sz to %d (%d)",
                   firstpp->cat, maxsize);
         }
         if  ((firstpp->cat >= firstpp->is->max_cat) &&
                 (diffidx + codelen > maxsize) )
         { /* max size - can't help, need to merge it */
             if (is->method->debug >7)
                yaz_log(YLOG_LOG,"isamd_appd: need to merge");
             if (is->method->debug >9)  /* !!!!! */
                yaz_log(YLOG_LOG,"isamd_appd: going to merge with m=%d %d.%d",
                     i_mode, i_key.sysno, i_key.seqno);
             merge_rc = merge (firstpp, &i_key, filt, dictentry, dictlen);
             if (0!=merge_rc)
               return merge_rc;  /* merge handled them all ! */
             assert(!"merge returned zero ??");
         } /* need to merge */
      } /* block full */

      if (!( diffidx+codelen <= maxsize )) 
      { /* bug hunting */
         yaz_log(YLOG_LOG,"OOPS, diffidx problem: d=%d c=%d s=%d > m=%d",
           diffidx, codelen, diffidx+codelen, maxsize);
         yaz_log(YLOG_LOG,"ipos=%d f=%d=%d:%d",
           ipos, 
           isamd_addr(firstpp->pos, firstpp->cat),
           firstpp->cat, firstpp->pos );
      }
      assert ( diffidx+codelen <= maxsize );
      
      /* save the diff */ 
      memcpy(&(firstpp->buf[diffidx]),codebuff,codelen);
      diffidx += codelen;
      firstpp->size = firstpp->offset = diffidx;
      
      if (i_mode)
        firstpp->numKeys++; /* insert diff */
      else
        firstpp->numKeys--; /* delete diff */ 

      /* update length of this diff run */
      memcpy(&(firstpp->buf[difflenidx]),&diffidx,sizeof(diffidx));
      
      /* (try to) read the next input */
      i_ptr = i_item;
      i_more = filter_read(filt, &i_key, &i_mode); 
    /*  i_more = (*data->read_item)(data->clientData, &i_ptr, &i_mode); */
      if ( (i_more) && (is->method->debug >6) )
         yaz_log(YLOG_LOG,"isamd_appd: got m=%d %d.%d=%x.%x: %d",
            i_mode, 
            i_key.sysno, i_key.seqno, 
            i_key.sysno, i_key.seqno,
            i_key.sysno*2+i_mode);
   } /* more loop */

   /* clear the next difflen, if room for such */
   difflenidx = diffidx;
   while ( (difflenidx-diffidx<=sizeof(int)+1) && (difflenidx<maxsize))
     firstpp->buf[difflenidx++]='\0';

   if (firstpp->numKeys==0)
   { 
       /* FIXME: Release blocks that may be allocated !!! */
       return 0; /* don't bother storing this! */
   }

   dsize=diffidx-ISAMD_BLOCK_OFFSET_1;
   /* yaz_log(YLOG_LOG,"!! nxt=%d diffidx=%d ds=%d", 
           firstpp->next, diffidx, dsize);  */

   if ( (0==firstpp->next) && (dsize <ISAMD_MAX_DICT_LEN))
   {
        /* yaz_log(YLOG_LOG,"building a dict entry!!"); */
        assert(firstpp->numKeys < 128);
        assert(firstpp->numKeys >0);
        /* actually, 255 is good enough, but sign mismatches... */
        /* in real life, 4-5 is as much as we can hope for, as long */
        /* as ISAMD_MAX_DICT_LEN is reasonably small (8) */
        dictentry[0]=firstpp->numKeys;
        memcpy(dictentry+1, firstpp->buf+ISAMD_BLOCK_OFFSET_1, dsize);
        dictlen=dsize+1;
   }
   else 
   {
       if (0==firstpp->pos)  /* need to (re)alloc the block */
           firstpp->pos = isamd_alloc_block(is, firstpp->cat);
       retpos = save_first_pp( firstpp );
       isamd_pp_close(firstpp);
       dictentry[0]=0; /* mark as a real isam */
       memcpy(dictentry+1, &retpos, sizeof(ISAMD_P));
       dictlen=sizeof(ISAMD_P)+1;
   }
    
   return dictlen;
} /* append_diffs */




/*************************************************************
 * isamd_append itself
 *************************************************************/

int isamd_append (ISAMD is, char *dictentry, int dictlen, ISAMD_I data)
/*ISAMD_P isamd_append (ISAMD is, ISAMD_P ipos, ISAMD_I data) */
{
   FILTER F = filter_open(is,data);
   int newlen=0;

   if ( filter_isempty(F) ) /* can be, if del-ins of the same */
   {
      if (is->method->debug >3) 
         yaz_log(YLOG_LOG,"isamd_appd: nothing to do ");
      filter_close(F);
      ++(is->no_non);
      return dictlen; /* without doing anything at all */
   }

#ifdef SKIPTHIS 
   /* The old way to handle singletons */
   if ( ( 0==ipos) && filter_only_one(F) )
   {
      struct it_key k;
      int mode;
      filter_read(F,&k,&mode);     
      assert(mode); 
      rc = singleton_encode(&k);
      if (!rc) 
      {
      if (is->method->debug >9) 
         yaz_log(YLOG_LOG,"isamd_appd: singleton didn't fit, backfilling");
         filter_backfill(F,&k, mode);
      }
      if (is->method->debug >9) 
         yaz_log(YLOG_LOG,"isamd_appd: singleton %d (%x)",
           rc,rc);
      if (rc)
        is->no_singles++;
      assert ( (rc==0) || is_singleton(rc) );
   }
   newlen = append_diffs(is,ipos,F); 
#endif
   newlen = append_diffs(is,dictentry,dictlen,F); 
   filter_close(F);

   if (is->method->debug >2) 
      yaz_log(YLOG_LOG,"isamd_appd: ret len=%d ", newlen);
   return newlen;
} /*  isamd_append */







/*
 * $Log: merge-d.c,v $
 * Revision 1.30.2.3  2006-12-05 21:14:42  adam
 * Updated code to use new YAZ log functions/defines.
 *
 * Revision 1.30.2.2  2006/10/27 11:06:47  adam
 * Fixed several compilation warnings. (gcc 4.1.2, -O3 -g -Wall)
 *
 * Revision 1.30.2.1  2006/08/14 10:39:11  adam
 * Update copyright year + FSF address
 *
 * Revision 1.30  2003/03/05 16:41:10  adam
 * Fix GCC warnings
 *
 * Revision 1.29  2002/11/26 22:18:34  adam
 * Remove // comments
 *
 * Revision 1.28  2002/08/02 19:26:56  adam
 * Towards GPL
 *
 * Revision 1.27  2002/07/12 18:12:21  heikki
 * Isam-D now stores small entries directly in the dictionary.
 * Needs more tuning and cleaning...
 *
 * Revision 1.26  2002/07/11 16:16:00  heikki
 * Fixed a bug in isamd, failed to store a single key when its bits
 * did not fit into a singleton.
 *
 * Revision 1.25  1999/11/30 13:48:04  adam
 * Improved installation. Updated for inclusion of YAZ header files.
 *
 * Revision 1.24  1999/10/05 09:57:40  heikki
 * Tuning the isam-d (and fixed a small "detail")
 *
 * Revision 1.23  1999/09/27 14:36:36  heikki
 * singletons
 *
 * Revision 1.22  1999/09/23 18:01:18  heikki
 * singleton optimising
 *
 * Revision 1.21  1999/09/21 17:36:43  heikki
 * Added filter function. Not much of effect on the small test set...
 *
 * Revision 1.20  1999/09/20 15:48:06  heikki
 * Small changes
 *
 * Revision 1.19  1999/09/13 13:28:28  heikki
 * isam-d optimizing: merging input data in the same go
 *
 * Revision 1.18  1999/08/25 18:09:24  heikki
 * Starting to optimize
 *
 * Revision 1.17  1999/08/24 13:17:42  heikki
 * Block sizes, comments
 *
 * Revision 1.16  1999/08/24 10:12:02  heikki
 * Comments about optimising
 *
 * Revision 1.15  1999/08/22 08:26:34  heikki
 * COmments
 *
 * Revision 1.14  1999/08/20 12:25:58  heikki
 * Statistics in isamd
 *
 * Revision 1.13  1999/08/18 13:59:19  heikki
 * Fixed another unlikely difflen bug
 *
 * Revision 1.12  1999/08/18 13:28:17  heikki
 * Set log levels to decent values
 *
 * Revision 1.11  1999/08/18 10:37:11  heikki
 * Fixed (another) difflen bug
 *
 * Revision 1.10  1999/08/18 09:13:31  heikki
 * Fixed a detail
 *
 * Revision 1.9  1999/08/17 19:46:53  heikki
 * Fixed a memory leak
 *
 * Revision 1.8  1999/08/07 11:30:59  heikki
 * Bug fixing (still a mem leak somewhere)
 *
 * Revision 1.7  1999/08/04 14:21:18  heikki
 * isam-d seems to be working.
 *
 * Revision 1.6  1999/07/23 15:43:05  heikki
 * Hunted a few bugs in isam-d. Still crashes on the long test run
 *
 * Revision 1.5  1999/07/23 13:58:52  heikki
 * merged closer to working, still fails on filling a separate, large block
 *
 * Revision 1.4  1999/07/21 14:53:55  heikki
 * isamd read and write functions work, except when block full
 * Merge missing still. Need to split some functions
 *
 * Revision 1.1  1999/07/14 13:14:47  heikki
 * Created empty
 *
 *
 */



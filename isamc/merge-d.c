/*
 * Copyright (c) 1996-1998, Index Data.
 * See the file LICENSE for details.
 * Heikki Levanto
 *
 * $Id: merge-d.c,v 1.12 1999-08-18 13:28:17 heikki Exp $
 *
 * todo
 *  - Clean up log levels
 *  - Input filter: Eliminate del-ins pairs, tell if only one entry (or none)
 *  - single-entry optimizing
 *  - study and optimize block sizes (later)
 *  - Clean up the different ways diffs are handled in writing and reading
 *  - Keep a merge-count in the firstpp, and if the block has already been
 *    merged, reduce it to a larger size even if it could fit in a small one!
 *  - Keep minimum freespace in the category table, and use that in reduce!
 *
 * bugs
 *  - Still has not been able to run a complete long test on bagel!
 *
 * caveat
 *  There is a confusion about the block addresses. cat or type is the category,
 *  pos or block is the block number. pp structures keep these two separate,
 *  and combine when saving the pp. The next pointer in the pp structure is
 *  also a combined address, but needs to be combined every time it is needed,
 *  and separated when the partss are needed... This is done with the isamd_
 *  _block, _type, and _addr macros. The _addr takes block and type as args,
 *  in that order. This conflicts with the order these are often mentioned in 
 *  the debug log calls, and other places, leading to small mistakes here
 *  and there. 
 *
 *  Needs cleaning! The way diff blocks are handled in append and reading is
 *  quite different, and likely to give maintenance problems.
 *
 *  log levels (set isamd=x in zebra.cfg (or what ever cfg file you use) )
 *  NOT IMPLEMEMTED YET !!!
 *    0 = no logging. Default
 *    1 = no logging here. isamd logs overall statistics
 *    2 = Each call to isamd_append with start address and no more
 *    3 = Start and type of append, start of merge, and result of append
 *    4 = Block allocations
 *    5 = Block-level operations (read/write)
 *    6 = Details about diff blocks etc.
 *    7 = Log each record as it passes the system (once)
 *    8 = Log raw and (de)coded data
 *    9 = Anything else that may be useful
 *   .. = Anything needed toi hunt a specific bug
 *  (note that all tests in the code are like debug>3, which means 4 or above!)
 */

#include <stdlib.h>
#include <assert.h>
#include <string.h>
#include <stdio.h>
#include <log.h>
#include "../index/index.h"
#include "isamd-p.h"


struct ISAMD_DIFF_s {
  int diffidx;
  int maxidx;
  struct it_key key;
  void *decodeData;
  int mode;
};



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


static int separateDiffBlock(ISAMD_PP pp)
{
  if (pp->next) 
     return 1;  /* multi-block chains always have a separate diff block */
  return ( pp->size + 2*sizeof(int) > pp->is->method->filecat[pp->cat].bsize);
  /* make sure there is at least room for the length and one diff. if not, */
  /* it goes to a separate block */
  
  /* todo: Make the limit adjustable in the filecat table ! */
}


/**************************************************************
 * Reading 
 **************************************************************/

static void getDiffInfo(ISAMD_PP pp, int diffidx)
{ /* builds the diff info structures from a diffblock */
   int maxinfos = pp->is->method->filecat[pp->cat].bsize / 5 +1;
    /* Each diff takes at least 5 bytes. Probably more, but this is safe */
   int i=1;  /* [0] is used for the main data */
   int diffsz= maxinfos * sizeof(struct ISAMD_DIFF_s);

   pp->diffinfo = xmalloc( diffsz ); 
   memset(pp->diffinfo,'\0',diffsz);
   if (pp->is->method->debug > 5)   
      logf(LOG_LOG,"isamd_getDiffInfo: %d (%d:%d), ix=%d mx=%d",
         isamd_addr(pp->pos, pp->cat), pp->cat, pp->pos, diffidx,maxinfos);
   assert(pp->diffbuf);

   while (i<maxinfos) 
   {  
      if ( diffidx+sizeof(int) > pp->is->method->filecat[pp->cat].bsize )
      {
         if (pp->is->method->debug > 5)
           logf(LOG_LOG,"isamd_getDiffInfo:Near end (no room for len) at ix=%d n=%d",
               diffidx, i);
         return; /* whole block done */
      }
      memcpy( &pp->diffinfo[i].maxidx, &pp->diffbuf[diffidx], sizeof(int) );

      if (pp->is->method->debug > 5)
        logf(LOG_LOG,"isamd_getDiffInfo: max=%d ix=%d dbuf=%p",
          pp->diffinfo[i].maxidx, diffidx, pp->diffbuf);

      if ( (pp->is->method->debug > 0) &&
         (pp->diffinfo[i].maxidx > pp->is->method->filecat[pp->cat].bsize) )
      { /* bug-hunting, this fails on some long runs that log too much */
         logf(LOG_LOG,"Bad MaxIx!!! %s:%d: diffidx=%d", 
                       __FILE__,__LINE__, diffidx);
         logf(LOG_LOG,"i=%d maxix=%d bsz=%d", i, pp->diffinfo[i].maxidx,
                       pp->is->method->filecat[pp->cat].bsize);
         logf(LOG_LOG,"pp=%d=%d:%d  pp->nx=%d=%d:%d",
                       isamd_addr(pp->pos,pp->cat), pp->pos, pp->cat,
                       pp->next, isamd_type(pp->next), isamd_block(pp->next) );                      
      }
      assert(pp->diffinfo[i].maxidx <= pp->is->method->filecat[pp->cat].bsize+1);

      if (0==pp->diffinfo[i].maxidx)
      {
         if (pp->is->method->debug > 5)  //!!! 4
           logf(LOG_LOG,"isamd_getDiffInfo:End mark at ix=%d n=%d",
               diffidx, i);
         return; /* end marker */
      }
      diffidx += sizeof(int);
      pp->diffinfo[i].decodeData = (*pp->is->method->code_start)(ISAMD_DECODE);
      pp->diffinfo[i].diffidx = diffidx;
      if (pp->is->method->debug > 5)
        logf(LOG_LOG,"isamd_getDiff[%d]:%d-%d %s",
          i,diffidx-sizeof(int),pp->diffinfo[i].maxidx,
          hexdump((char *)&pp->diffbuf[diffidx-4],8,0) );
      diffidx=pp->diffinfo[i].maxidx;
      if ( diffidx > pp->is->method->filecat[pp->cat].bsize )
        return; /* whole block done */
      ++i;
   }
   assert (!"too many diff sequences in the block");
}

static void loadDiffs(ISAMD_PP pp)
{  /* assumes pp is a firstblock! */
   int diffidx;
   int diffaddr;
   if (0==pp->diffs)
     return; /* no diffs to talk about */
   if (pp->diffs & 1 )
   { /* separate diff block, load it */
      pp->diffbuf= xmalloc( pp->is->method->filecat[pp->cat].bsize);
      diffaddr=isamd_addr(pp->diffs/2, pp->cat);
      isamd_read_block (pp->is, isamd_type(diffaddr), 
                                isamd_block(diffaddr), pp->diffbuf );
      diffidx= ISAMD_BLOCK_OFFSET_N; 
      if (pp->is->method->debug > 4)
        logf(LOG_LOG,"isamd_LoadDiffs: loaded block %d=%d:%d, d=%d ix=%d",
          diffaddr, isamd_type(diffaddr),isamd_block(diffaddr), 
          pp->diffs,diffidx);
   }
   else
   { /* integrated block, just set the pointers */
     pp->diffbuf = pp->buf;
     diffidx = pp->size;  /* size is the beginning of diffs, diffidx the end*/
      if (pp->is->method->debug > 4)
        logf(LOG_LOG,"isamd_LoadDiffs: within %d=%d:%d, d=%d ix=%d ",
          isamd_addr(pp->pos,pp->cat), pp->cat, pp->pos, pp->diffs, diffidx);
   }
   getDiffInfo(pp,diffidx);
} /* loadDiffs */


void isamd_free_diffs(ISAMD_PP pp)
{
  int i;
  if (pp->is->method->debug > 5)
     logf(LOG_LOG,"isamd_free_diffs: pp=%p di=%p", pp, pp->diffinfo);
  if (!pp->diffinfo) 
    return;
  for (i=1;pp->diffinfo[i].decodeData;i++) 
  {
      if (pp->is->method->debug > 8)
         logf(LOG_LOG,"isamd_free_diffs [%d]=%p",i, 
                       pp->diffinfo[i].decodeData);
      (*pp->is->method->code_stop)(ISAMD_DECODE,pp->diffinfo[i].decodeData);
  } 
  xfree(pp->diffinfo);
  if (pp->diffbuf != pp->buf)
    xfree (pp->diffbuf);  
} /* isamd_free_diffs */


/* Reads one item and corrects for the diffs, if any */
/* return 1 for ok, 0 for eof */
int isamd_read_item (ISAMD_PP pp, char **dst)
{
  char *keyptr;
  char *codeptr;
  char *codestart;
  int winner=0; /* which diff holds the day */
  int i; /* looping diffs */
  int cmp;
  int retry=1;
  if (pp->diffs==0)  /* no diffs, just read the thing */
     return isamd_read_main_item(pp,dst);

  if (!pp->diffinfo)
    loadDiffs(pp);
  while (retry)
  {
     retry=0;
     if (0==pp->diffinfo[0].key.sysno) 
     { /* 0 is special case, main data. */
        keyptr=(char*) &(pp->diffinfo[0].key);
        pp->diffinfo[0].mode = ! isamd_read_main_item(pp,&keyptr);
        if (pp->is->method->debug > 7)
          logf(LOG_LOG,"isamd_read_item: read main %d.%d (%x.%x)",
            pp->diffinfo[0].key.sysno, pp->diffinfo[0].key.seqno,
            pp->diffinfo[0].key.sysno, pp->diffinfo[0].key.seqno);
     } /* get main data */
     winner = 0;
     for (i=1; (!retry) && (pp->diffinfo[i].decodeData); i++)
     {
        if (pp->is->method->debug > 8)
          logf(LOG_LOG,"isamd_read_item: considering d%d %d.%d ix=%d mx=%d",
               i, pp->diffinfo[i].key.sysno, pp->diffinfo[i].key.seqno,
                  pp->diffinfo[i].diffidx,   pp->diffinfo[i].maxidx);
            
        if ( (0==pp->diffinfo[i].key.sysno) &&
             (pp->diffinfo[i].diffidx < pp->diffinfo[i].maxidx))
        {/* read a new one, if possible */
           codeptr= codestart = &(pp->diffbuf[pp->diffinfo[i].diffidx]);
           keyptr=(char *)&(pp->diffinfo[i].key);
           (*pp->is->method->code_item)(ISAMD_DECODE,
                 pp->diffinfo[i].decodeData, &keyptr, &codeptr);
           pp->diffinfo[i].diffidx += codeptr-codestart;
           pp->diffinfo[i].mode = pp->diffinfo[i].key.seqno & 1;
           pp->diffinfo[i].key.seqno = pp->diffinfo[i].key.seqno >>1 ;
           if (pp->is->method->debug > 7)
             logf(LOG_LOG,"isamd_read_item: read diff[%d] %d.%d (%x.%x)",i,
               pp->diffinfo[i].key.sysno, pp->diffinfo[i].key.seqno,
               pp->diffinfo[i].key.sysno, pp->diffinfo[i].key.seqno);
        } 
        if ( 0!= pp->diffinfo[i].key.sysno)
        { /* got a key, compare */
          cmp=key_compare(&pp->diffinfo[i].key, &pp->diffinfo[winner].key);
          if (0==pp->diffinfo[winner].key.sysno)
            cmp=-1; /* end of main sequence, take all diffs */
          if (cmp<0)
          {
             if (pp->is->method->debug > 8)
               logf(LOG_LOG,"isamd_read_item: ins %d<%d %d.%d (%x.%x) < %d.%d (%x.%x)",
                 i, winner,
                 pp->diffinfo[i].key.sysno, pp->diffinfo[i].key.seqno,
                 pp->diffinfo[i].key.sysno, pp->diffinfo[i].key.seqno,
                 pp->diffinfo[winner].key.sysno, pp->diffinfo[winner].key.seqno,
                 pp->diffinfo[winner].key.sysno, pp->diffinfo[winner].key.seqno);
             if (pp->diffinfo[i].mode)  /* insert diff, should always be */
               winner = i;
             else
               assert(!"delete diff for nonexisting item");  
               /* is an assert too steep here? Not really.*/
          } /* earlier key */
          else if (cmp==0)
          {
             if (!pp->diffinfo[i].mode) /* delete diff. should always be */
             {
                if (pp->is->method->debug > 8)
                  logf(LOG_LOG,"isamd_read_item: del %d at%d %d.%d (%x.%x)",
                    i, winner,
                    pp->diffinfo[i].key.sysno, pp->diffinfo[i].key.seqno,
                    pp->diffinfo[i].key.sysno, pp->diffinfo[i].key.seqno);
                pp->diffinfo[winner].key.sysno=0; /* delete it */
             }
             else
                if (pp->is->method->debug > 2)
                  logf(LOG_LOG,"isamd_read_item: duplicate ins %d at%d %d.%d (%x.%x)",
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
      logf(LOG_LOG,"isamd_read_item: got %d  %d.%d (%x.%x)",
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
      logf(LOG_LOG,"isamd_read_item: eof w=%d  %d.%d (%x.%x)",
        winner,
        pp->diffinfo[winner].key.sysno, pp->diffinfo[winner].key.seqno,
        pp->diffinfo[winner].key.sysno, pp->diffinfo[winner].key.seqno);
    assert(winner==0); /* if nothing found, nothing comes from a diff */
    cmp= 0; /* eof */
  }
  return cmp;
   
} /* isamd_read_item */

/*****************************************************************
 * Support routines 
 *****************************************************************/

static void isamd_reduceblock(ISAMD_PP pp)
/* takes a large block, and reduces its category if possible */
/* Presumably the first block in an isam-list */
{
   if (pp->pos)
      return; /* existing block, do not touch */
   if (pp->is->method->debug > 5)  
     logf(LOG_LOG,"isamd_reduce: start p=%d c=%d sz=%d",
       pp->pos, pp->cat, pp->size); 
   while ( ( pp->cat > 0 ) && (!pp->next) && 
           (pp->offset < pp->is->method->filecat[pp->cat-1].bsize ) )
      pp->cat--;
   pp->pos = isamd_alloc_block(pp->is, pp->cat);
   if (pp->is->method->debug > 5) 
     logf(LOG_LOG,"isamd_reduce:  got  p=%d c=%d sz=%d",
       pp->pos, pp->cat, pp->size);    
} /* reduceblock */



 
static int save_first_pp ( ISAMD_PP firstpp)
{
   isamd_reduceblock(firstpp);
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

static ISAMD_PP read_diff_block(ISAMD_PP firstpp, int* p_diffidx)
{ /* reads the diff block (if separate) and sets diffidx right */
   ISAMD_PP pp=firstpp;
   int i;
   int diffidx;   
   if (pp->diffs == 0)
   { /* no diffs yet, create room for them */
      if (separateDiffBlock(firstpp))
      { /* create a new block */
         pp=isamd_pp_open(pp->is,isamd_addr(0,firstpp->cat));
         pp->pos = isamd_alloc_block(pp->is, pp->cat);
         firstpp->diffs = pp->pos*2 +1;
         diffidx = pp->size = pp->offset = ISAMD_BLOCK_OFFSET_N;
         if (pp->is->method->debug >5) 
             logf(LOG_LOG,"isamd_appd: alloc diff  (d=%d) %d=%d:%d ix=%d",
                   firstpp->diffs,
                   isamd_addr(pp->pos,pp->cat), pp->cat, pp->pos, 
                   diffidx);
      }
      else
      { /* prepare to append diffs in head */
        diffidx = pp->size;
        pp->diffs = diffidx *2 +0; 
        i=diffidx; /* make an end marker */
        while ( ( i < pp->is->method->filecat[pp->cat].bsize) && 
                ( i <= diffidx + sizeof(int)))
            pp->buf[i++]='\0';
        if (pp->is->method->debug >5) 
             logf(LOG_LOG,"isamd_appd: set up diffhead  (d=%d) %d=%d:%d ix=%d",
                   firstpp->diffs,
                   isamd_addr(pp->pos,pp->cat), pp->cat, pp->pos, 
                   diffidx);
      }
   } /* new */
   else
   { /* existing diffs */
      if (pp->diffs & 1)
      { /* diffs in a separate block, load it */
        pp=isamd_pp_open(pp->is, isamd_addr(firstpp->diffs/2,pp->cat));
        diffidx = pp->offset= pp->size;
        if (pp->is->method->debug >5) 
           logf(LOG_LOG,"isamd_appd: loaded diff (d=%d) %d=%d:%d ix=%d",
                 firstpp->diffs,
                 isamd_addr(pp->pos,pp->cat), pp->cat, pp->pos, 
                 diffidx);
      }
      else
      { /* diffs within the nead */
         diffidx= pp->diffs/2;
         if (pp->is->method->debug >5)  
            logf(LOG_LOG,"isamd_appd: diffs in head d=%d %d=%d:%d ix=%d sz=%d",
                     pp->diffs,
                     isamd_addr(pp->pos,pp->cat), pp->cat, pp->pos, 
                     diffidx, pp->size);
      }
   } /* diffs exist already */
   *p_diffidx = diffidx;
   return pp;
} /* read_diff_block */




/*******************************************************************
 * Building main blocks (no diffs)
 *******************************************************************/



static ISAMD_PP get_new_main_block( ISAMD_PP firstpp, ISAMD_PP pp)
{  /* allocates a new block for the main data, and links it in */
   int newblock;
   if (firstpp==pp) 
   { /* special case: it was the first block. Save much later */
      if (0==firstpp->pos)
      { /* firstpp not allocated yet, do so now, */
        /* to keep blocks in order. Don't save yet, though */
         firstpp->pos = isamd_alloc_block(pp->is, firstpp->cat);
      }
      newblock = isamd_alloc_block(pp->is, firstpp->cat);
      firstpp->next = isamd_addr(newblock,firstpp->cat);
        /* keep the largest category */
      pp=isamd_pp_open(pp->is,isamd_addr(0,firstpp->cat));/*don't load*/
      pp->pos=newblock; 
      pp->size = pp->offset = ISAMD_BLOCK_OFFSET_N; 
      pp->next=0;
      if (pp->is->method->debug >3) 
         logf(LOG_LOG,"isamd_g_mainblk: Alloc2 f=%d=%d:%d n=%d=%d:%d",
            isamd_addr(firstpp->pos,firstpp->cat), 
            firstpp->cat, firstpp->pos,
            isamd_addr(pp->pos,pp->cat), pp->cat, pp->pos );
   }
   else
   { /* it was not the first block */
      newblock = isamd_alloc_block(pp->is, firstpp->cat);
      pp->next = isamd_addr(newblock,firstpp->cat);
      if (pp->is->method->debug >3) 
         logf(LOG_LOG,"isamd_build: Alloc1 after p=%d=%d:%d->%d=%d:%d",
            isamd_addr(pp->pos,pp->cat), pp->cat, pp->pos,
            isamd_addr(newblock,pp->cat), pp->cat, newblock );
      isamd_buildlaterblock(pp);
      isamd_write_block(pp->is,pp->cat,pp->pos,pp->buf);
      pp->size = pp->offset = ISAMD_BLOCK_OFFSET_N;
      pp->next=0;
      pp->cat = firstpp->cat;
      pp->pos = newblock;
      pp->cat = firstpp->cat; /* is already, never mind */
   }
  return pp;
} /* get_new_main_block */


static ISAMD_PP  append_main_item(ISAMD_PP firstpp, 
                                  ISAMD_PP pp, 
                                  struct it_key *i_key,
                                  void *encoder_data)
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
   (*pp->is->method->code_item)(ISAMD_ENCODE, encoder_data, &c_ptr, &i_ptr);
   codelen = c_ptr - codebuff;
   assert ( (codelen<128) && (codelen>0));
   if (pp->is->method->debug >7)
      logf(LOG_LOG,"isamd:build: coded into %s  (nk=%d)",
          hexdump(codebuff, c_ptr-codebuff,hexbuff), firstpp->numKeys+1);

   if (pp->offset + codelen > maxsize )
   { /* oops, block full - get a new one */
      pp =  get_new_main_block( firstpp, pp );
      /* reset encoging and code again */
      (*pp->is->method->code_reset)(encoder_data);
      c_ptr=codebuff;
      i_ptr=i_item;
      (*pp->is->method->code_item)(ISAMD_ENCODE, encoder_data, &c_ptr, &i_ptr);
      codelen = c_ptr - codebuff;
      assert ( (codelen<128) && (codelen>0));
      if (pp->is->method->debug >7)
         logf(LOG_LOG,"isamd:build: recoded into %s  (nk=%d)",
             hexdump(codebuff, c_ptr-codebuff,hexbuff), firstpp->numKeys+1);
   } /* block full */    
   
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


static int isamd_build_first_block(ISAMD is, ISAMD_I data) 
{
   struct it_key i_key;  /* input key */
   char *i_item= (char *) &i_key;  /* same as char */
   char *i_ptr=i_item;
   int i_more =1;
   int i_mode;     /* 0 for delete, 1 for insert */ 

   ISAMD_PP firstpp;
   ISAMD_PP pp;
   void *encoder_data;
   
   char hexbuff[64];
   
   firstpp=pp=isamd_pp_open(is, isamd_addr(0,is->max_cat));
   firstpp->size = firstpp->offset = ISAMD_BLOCK_OFFSET_1;
   
   encoder_data=(*is->method->code_start)(ISAMD_ENCODE);
   
   if (is->method->debug >2)
      logf(LOG_LOG,"isamd_bld start: p=%d=%d:%d sz=%d maxsz=%d ",
         isamd_addr(pp->pos,pp->cat), pp->cat, pp->pos, 
         pp->size, pp->is->method->filecat[pp->is->max_cat].bsize); 

   /* read first input */
   i_ptr = i_item;
   i_more = (*data->read_item)(data->clientData, &i_ptr, &i_mode); 
   if (i_more)
     assert( i_ptr-i_item == sizeof(i_key) );

   if (pp->is->method->debug >7)
      logf(LOG_LOG,"isamd: build_fi start: m=%d %s",
         i_mode, hexdump(i_item,i_ptr-i_item,hexbuff) );

   while (i_more)
   { 
      if (i_mode!=0) 
      { /* ignore deletes here, should not happen */
         pp= append_main_item(firstpp, pp, &i_key, encoder_data);      
      } /* not delete */
      
      /* (try to) read the next item */
      i_ptr = i_item;
      i_more = (*data->read_item)(data->clientData, &i_ptr, &i_mode); 
      
      if ( (i_more) && (pp->is->method->debug >7) )
         logf(LOG_LOG,"isamd: build_fi read: m=%d %s",
            i_mode, hexdump(i_item,i_ptr-i_item,hexbuff) );
      
   } /* i_more */
   (*is->method->code_stop)(ISAMD_ENCODE, encoder_data);

   return save_both_pps( firstpp, pp );

} /* build_first_block */


/***************************************************************
 * Merging diffs 
 ***************************************************************/

 
static int merge ( ISAMD_PP *p_firstpp,   /* first pp of the chain */
                   ISAMD_PP *p_pp,        /* diff block */
                   struct it_key *p_key ) /* not used yet */
{
  ISAMD_PP readpp = *p_firstpp;
  int diffidx;  
  int killblk=0;
  struct it_key r_key;
  char * r_ptr;
  int r_more = 1;
  ISAMD_PP firstpp;  /* the new first, the one we write into */
  ISAMD_PP pp;
  void *encoder_data;
     
  /* set up diffs as they should be for reading */
  readpp->offset= ISAMD_BLOCK_OFFSET_1; 
  
  if ( (*p_firstpp)->diffs & 1 )
  { /* separate diff block in *p_pp */
     killblk = readpp->diffs/2;
     diffidx /*size*/ = readpp->is->method->filecat[readpp->cat].bsize; 
     readpp->diffbuf= xmalloc( diffidx); /* copy diffs to where read wants*/
     memcpy( readpp->diffbuf, &((*p_pp)->buf[0]), diffidx);
     diffidx = ISAMD_BLOCK_OFFSET_N;
     if (readpp->is->method->debug >2) 
     { 
        logf(LOG_LOG,"isamd_merge:separate diffs at ix=%d", 
                diffidx);
        logf(LOG_LOG,"isamd_merge: dbuf=%p (from %p) pp=%p", 
                 readpp->diffbuf, &((*p_pp)->buf[0]), (*p_pp) );
     }
  }
  else
  { /* integrated diffs */
     assert ( *p_pp == *p_firstpp );  /* can only be in the first block */
     diffidx=readpp->size;
     readpp->diffs = diffidx*2+0;
     readpp->diffbuf=readpp->buf; 
     if (readpp->is->method->debug >2)  
         logf(LOG_LOG,"isamd_merge:local diffs at %d: %s", 
           diffidx,hexdump(&(readpp->diffbuf[diffidx]),8,0));
  }

  getDiffInfo(readpp,diffidx);
  if (readpp->is->method->debug >8) 
         logf(LOG_LOG,"isamd_merge: diffinfo=%p", readpp->diffinfo);
  

  if (killblk) 
  {  /* we had a separate diff block, release it, we have copied the data */
     isamd_release_block(readpp->is, readpp->cat, killblk);
     isamd_pp_close (*p_pp);
     if (readpp->is->method->debug >3)  
         logf(LOG_LOG,"isamd_merge: released diff block %d=%d:%d",
              isamd_addr(killblk,readpp->cat), readpp->cat, killblk );
  }


  /* release our data block. Do before reading, when pos is stable ! */
  killblk=readpp->pos;
  assert(killblk);
  isamd_release_block(readpp->is, readpp->cat, killblk);
  if (readpp->is->method->debug >3)   
      logf(LOG_LOG,"isamd_merge: released old firstblock %d (%d:%d)",
               isamd_addr(killblk,readpp->cat), readpp->cat, killblk );

  r_ptr= (char *) &r_key;
  r_more = isamd_read_item( readpp, &r_ptr);
  if (!r_more)  
  { /* oops, all data has been deleted! what to do??? */
    /* never mind, we have at least one more delta to add to the block */
    /* pray that is not a delete as well... */
    r_key.sysno = 0;
    r_key.seqno = 0;
     if (readpp->is->method->debug >3) 
         logf(LOG_LOG,"isamd_merge:all data has been deleted (nk=%d) ",
            readpp->numKeys);
    assert (readpp->numKeys == 0);
  }


  /* set up the new blocks for simple writing */
  firstpp=pp=isamd_pp_open(readpp->is,isamd_addr(0, readpp->is->max_cat));
  firstpp->size = firstpp->offset = ISAMD_BLOCK_OFFSET_1;
  encoder_data = (*pp->is->method->code_start)(ISAMD_ENCODE);
  
  while (r_more)
  {
     if (readpp->is->method->debug >6) 
         logf(LOG_LOG,"isamd_merge: got key %d.%d",
           r_key.sysno, r_key.seqno );
     pp= append_main_item(firstpp, pp, &r_key, encoder_data);

     if ( (readpp->pos != killblk ) && (0!=readpp->pos) )
     {  /* pos can get to 0 at end of main seq, if still diffs left...*/
        if (readpp->is->method->debug >3)  
            logf(LOG_LOG,"isamd_merge: released block %d (%d:%d) now %d=%d:%d",
                isamd_addr(killblk,readpp->cat), readpp->cat, killblk,
                isamd_addr(readpp->pos,readpp->cat),readpp->cat, readpp->pos );
        isamd_release_block(readpp->is, readpp->cat, readpp->pos);
        killblk=readpp->pos;
     }

     /* (try to) read next item */
     r_ptr= (char *) &r_key;
     r_more = isamd_read_item( readpp, &r_ptr);

  } /* while read */
  
  /* TODO: while pkey is an insert, and after last key inserted, append it */
  /* will prevent multiple merges on large insert runs */  

  /* set things up so that append can continue */
  isamd_reduceblock(firstpp);
  firstpp->diffs=0; 

  if (firstpp!=pp) 
  { /* the last data block is of no interest any more */
    save_last_pp(pp);
    if (readpp->is->method->debug >4) 
        logf(LOG_LOG,"isamd_merge: saved last block %d=%d:%d",
              isamd_addr(pp->pos,pp->cat), pp->cat, pp->pos);
    isamd_pp_close(pp);
  }

  if (readpp->is->method->debug >5) 
        logf(LOG_LOG,"isamd_merge: closing readpp %d=%d:%d di=%p",
              isamd_addr(readpp->pos,readpp->cat), readpp->cat, readpp->pos,
              readpp->diffinfo);
  isamd_pp_close(readpp); /* pos is 0 by now, at eof. close works anyway */

  (*firstpp->is->method->code_stop)(ISAMD_ENCODE, encoder_data);
  
  *p_firstpp = firstpp; 

  if (readpp->is->method->debug >2)  
      logf(LOG_LOG,"isamd_merge: merge ret  %d=%d:%d nx=%d=%d:%d d=%d=2*%d+%d",
            isamd_addr(pp->pos,pp->cat), pp->cat, pp->pos,
            pp->next, isamd_type(pp->next), isamd_block(pp->next),
            pp->diffs, pp->diffs/2, pp->diffs &1 );
  return 0; 
  
} /* merge */



/***************************************************************
 * Appending diffs 
 ***************************************************************/



static int append_diffs(ISAMD is, ISAMD_P ipos, ISAMD_I data)
{
   struct it_key i_key;    /* one input item */
   char *i_item = (char *) &i_key;  /* same as chars */
   char *i_ptr=i_item;
   int i_more =1;
   int i_mode;     /* 0 for delete, 1 for insert */ 

   ISAMD_PP firstpp;
   ISAMD_PP pp;
   void *encoder_data;
   char hexbuff[64];
   int diffidx=0;
   int maxsize=0;
   int difflenidx;
   char codebuff[128];
   char *c_ptr = codebuff;
   int codelen;
   int merge_rc;

   firstpp=isamd_pp_open(is, ipos);
   if (is->method->debug >2) 
      logf(LOG_LOG,"isamd_appd: Start ipos=%d=%d:%d d=%d=%d*2+%d nk=%d",
        ipos, isamd_type(ipos), isamd_block(ipos),
        firstpp->diffs, firstpp->diffs/2, firstpp->diffs & 1, firstpp->numKeys);
   pp=read_diff_block(firstpp, &diffidx);
   encoder_data=(*is->method->code_start)(ISAMD_ENCODE);
   maxsize = is->method->filecat[pp->cat].bsize; 
   
   difflenidx = diffidx;
   diffidx+=sizeof(int);  /* difflen will be stored here */
   
   /* read first input */
   i_ptr = i_item;
   i_more = (*data->read_item)(data->clientData, &i_ptr, &i_mode); 

   if (is->method->debug >6)
      logf(LOG_LOG,"isamd_appd: start with m=%d %s",
         i_mode, hexdump(i_item,i_ptr-i_item,hexbuff) );

   while (i_more)
   {     
      /* store the mode bit inside key */
      assert( ((i_key.seqno<<1)>>1) == i_key.seqno); /* can spare the bit */
      i_key.seqno = i_key.seqno * 2 + i_mode;  

      c_ptr=codebuff;
      i_ptr=i_item;
      (*is->method->code_item)(ISAMD_ENCODE, encoder_data, &c_ptr, &i_ptr);
      codelen = c_ptr - codebuff;
      assert ( (codelen<128) && (codelen>0));
      if (is->method->debug >7)
         logf(LOG_LOG,"isamd_appd: coded into %d: %s (nk=%d) (ix=%d)",
             codelen, hexdump(codebuff, codelen,hexbuff), 
             firstpp->numKeys,diffidx);

      if (diffidx + codelen > maxsize )
      { /* block full */
         if (is->method->debug >3)
            logf(LOG_LOG,"isamd_appd: block full (ix=%d mx=%d lix=%d)",
               diffidx, maxsize, difflenidx);
         if (is->method->debug >8)
            logf(LOG_LOG,"isamd_appd: block pp=%p buf=%p [%d]:%s",
               pp, pp->buf, 
               difflenidx, hexdump(&pp->buf[difflenidx],8,0));
         merge_rc = merge (&firstpp, &pp, &i_key);
         if (0!=merge_rc)
           return merge_rc;  /* merge handled them all ! */

         /* set things up so we can continue */
         pp = read_diff_block(firstpp, &diffidx);
         (*is->method->code_reset)(encoder_data);
         maxsize = is->method->filecat[pp->cat].bsize; 
         difflenidx=diffidx;
         diffidx+=sizeof(int);

         /* code the current input key again */
         c_ptr=codebuff;
         i_ptr=i_item;
         (*is->method->code_item)(ISAMD_ENCODE, encoder_data, &c_ptr, &i_ptr);
         codelen = c_ptr - codebuff;
         assert ( (codelen<128) && (codelen>0));
         if (is->method->debug >7)
            logf(LOG_LOG,"isamd_appd: recoded into %d: %s (nk=%d) (ix=%d)",
                codelen, hexdump(codebuff, codelen,hexbuff), 
                firstpp->numKeys,diffidx);
          
      } /* block full */
      
      /* save the diff */ 
      memcpy(&(pp->buf[diffidx]),codebuff,codelen);
      diffidx += codelen;
      if (i_mode)
        firstpp->numKeys++; /* insert diff */
      else
        firstpp->numKeys--; /* delete diff */ 

      /* update length of this diff run */
      memcpy(&(pp->buf[difflenidx]),&diffidx,sizeof(diffidx));
      if (firstpp==pp)
        firstpp->diffs =diffidx*2+0;
      else
        pp->size =diffidx;
      
      /* (try to) read the next input */
      i_ptr = i_item;
      i_more = (*data->read_item)(data->clientData, &i_ptr, &i_mode); 
      if ( (i_more) && (is->method->debug >6) )
         logf(LOG_LOG,"isamd_appd: got m=%d %s",
            i_mode, hexdump(i_item,i_ptr-i_item,hexbuff) );
   } /* more loop */

   /* clear the next difflen, if room for such */
   difflenidx = diffidx;
   while ( (difflenidx-diffidx<=sizeof(int)) && (difflenidx<maxsize))
     pp->buf[difflenidx++]='\0';


  (*firstpp->is->method->code_stop)(ISAMD_ENCODE, encoder_data);
   return save_both_pps( firstpp, pp );

} /* append_diffs */



/*************************************************************
 * isamd_append itself, Sweet, isn't it
 *************************************************************/

ISAMD_P isamd_append (ISAMD is, ISAMD_P ipos, ISAMD_I data)
{
   int retval=0;

   if (0==ipos)
      retval = isamd_build_first_block(is,data);
   else
      retval = append_diffs(is,ipos,data);

   if (0)  /*!*/ 
   {
     void *p1=xmalloc(100);
     void *p2=xmalloc(100);
     void *p3=xmalloc(100);
     logf(LOG_LOG,"Traversing xmalloc stuff. p1=%p p2=%p p3=%p",p1,p2,p3);
     xmalloc_trav("end of append"); /*!*/
     assert(!"foo");
   }
   
   return retval;
} /*  isamd_append */


/*
 * $Log: merge-d.c,v $
 * Revision 1.12  1999-08-18 13:28:17  heikki
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





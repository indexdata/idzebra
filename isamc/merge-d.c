/*
 * Copyright (c) 1996-1998, Index Data.
 * See the file LICENSE for details.
 * Heikki Levanto
 *
 * $Id: merge-d.c,v 1.5 1999-07-23 13:58:52 heikki Exp $
 *
 * todo
 *  - merge when needed
 *  - single-entry optimizing
 *  - study and optimize block sizes (later)
 *
 * bugs
 *  not yet ready
 *
 * caveat
 *  There is aconfusion about the block addresses. cat or type is the category,
 *  pos or block is the block number. pp structures keep these two separate,
 *  and combine when saving the pp. The next pointer in the pp structure is
 *  also a combined address, but needs to be combined every time it is needed,
 *  and separated when the partss are needed... This is done with the isamd_
 *  _block, _type, and _addr macros. The _addr takes block and type as args,
 *  in that order. This conflicts with the order these are often mentioned in 
 *  the debug log calls, and other places, leading to small mistakes here
 *  and there. 
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
   if (pp->is->method->debug > 4)
     logf(LOG_LOG,"isamd_getDiffInfo: %d (%d:%d), ix=%d mx=%d",
         isamd_addr(pp->pos, pp->cat), pp->cat, pp->pos, diffidx,maxinfos);
   assert(pp->diffbuf);

   while (i<maxinfos) 
   {  
      if ( diffidx+sizeof(int) > pp->is->method->filecat[pp->cat].bsize )
      {
         if (pp->is->method->debug > 4)
           logf(LOG_LOG,"isamd_getDiffInfo:Near end (no room for len) at ix=%d n=%d",
               diffidx, i);
         return; /* whole block done */
      }
      memcpy( &pp->diffinfo[i].maxidx, &pp->diffbuf[diffidx], sizeof(int) );

      if (pp->is->method->debug > 4)
        logf(LOG_LOG,"isamd_getDiffInfo: max=%d ix=%d dbuf=%p",
          pp->diffinfo[i].maxidx, diffidx, pp->diffbuf);

      if (0==pp->diffinfo[i].maxidx)
      {
         if (pp->is->method->debug > 4)
           logf(LOG_LOG,"isamd_getDiffInfo:End mark at ix=%d n=%d",
               diffidx, i);
         return; /* end marker */
      }
      diffidx += sizeof(int);
      pp->diffinfo[i].decodeData = (*pp->is->method->code_start)(ISAMD_DECODE);
      pp->diffinfo[i].diffidx = diffidx;
      if (pp->is->method->debug > 4)
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
  if (!pp->diffinfo) 
    return;
  for (i=0;pp->diffinfo[i].decodeData;i++) 
      (*pp->is->method->code_stop)(ISAMD_DECODE,pp->diffinfo[i].decodeData);    
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
        if (pp->is->method->debug > 4)
          logf(LOG_LOG,"isamd_read_item: read main %d.%d (%x.%x)",
            pp->diffinfo[0].key.sysno, pp->diffinfo[0].key.seqno,
            pp->diffinfo[0].key.sysno, pp->diffinfo[0].key.seqno);
     } /* get main data */
     winner = 0;
     for (i=1; (!retry) && (pp->diffinfo[i].decodeData); i++)
     {
        if (pp->is->method->debug > 4)
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
           if (pp->is->method->debug > 4)
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
             if (pp->is->method->debug > 4)
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
               /* is an assert too steep here?*/
          } /* earlier key */
          else if (cmp==0)
          {
             if (!pp->diffinfo[i].mode) /* delete diff. should always be */
             {
                if (pp->is->method->debug > 4)
                  logf(LOG_LOG,"isamd_read_item: del %d at%d %d.%d (%x.%x)",
                    i, winner,
                    pp->diffinfo[i].key.sysno, pp->diffinfo[i].key.seqno,
                    pp->diffinfo[i].key.sysno, pp->diffinfo[i].key.seqno);
                pp->diffinfo[winner].key.sysno=0; /* delete it */
             }
             else
                if (pp->is->method->debug > 4)
                  logf(LOG_LOG,"isamd_read_item: duplicate ins %d at%d %d.%d (%x.%x)",
                    i, winner,
                    pp->diffinfo[i].key.sysno, pp->diffinfo[i].key.seqno,
                    pp->diffinfo[i].key.sysno, pp->diffinfo[i].key.seqno);
                /* skip the insert, since we already have it in the base */
             pp->diffinfo[i].key.sysno=0; /* done with the delete */
             retry=1; /* start all over again */
          } /* matching key */
          /* else it is a later key, its turn will come */
        } /* got a key */
     } /* for each diff */
  } /* not retry */

  if ( pp->diffinfo[winner].key.sysno)
  {
    if (pp->is->method->debug > 4)
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
    if (pp->is->method->debug > 4)
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
   if (pp->is->method->debug > 2)
     logf(LOG_LOG,"isamd_reduce: start p=%d c=%d sz=%d",
       pp->pos, pp->cat, pp->size); 
   while ( ( pp->cat > 0 ) && (!pp->next) && 
           (pp->offset < pp->is->method->filecat[pp->cat-1].bsize ) )
      pp->cat--;
   pp->pos = isamd_alloc_block(pp->is, pp->cat);
   if (pp->is->method->debug > 2)
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
   int diffidx;   
   if (pp->diffs == 0)
   { /* no diffs yet, create room for them */
      if (separateDiffBlock(firstpp))
      { /* create a new block */
         pp=isamd_pp_open(pp->is,isamd_addr(0,firstpp->cat));
         pp->pos = isamd_alloc_block(pp->is, pp->cat);
         firstpp->diffs = pp->pos*2 +1;
         diffidx = pp->size = pp->offset = ISAMD_BLOCK_OFFSET_N;
         if (pp->is->method->debug >3) 
             logf(LOG_LOG,"isamd_appd: alloc diff  (d=%d) %d=%d:%d ix=%d",
                   firstpp->diffs,
                   isamd_addr(pp->pos,pp->cat), pp->cat, pp->pos, 
                   diffidx);
      }
      else
      { /* prepare to append diffs in head */
        diffidx = pp->size;
        pp->diffs = diffidx *2 +0;  
         if (pp->is->method->debug >3) 
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
        if (pp->is->method->debug >3) 
           logf(LOG_LOG,"isamd_appd: loaded diff (d=%d) %d=%d:%d ix=%d",
                 firstpp->diffs,
                 isamd_addr(pp->pos,pp->cat), pp->cat, pp->pos, 
                 diffidx);
      }
      else
      { /* diffs within the nead */
         diffidx= pp->diffs/2;
         if (pp->is->method->debug >3) 
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
         logf(LOG_LOG,"isamd_build: Alloc2 f=%d (%d:%d) n=%d(%d:%d)",
            isamd_addr(firstpp->pos,firstpp->cat), 
            firstpp->cat, firstpp->pos,
            isamd_addr(pp->pos,pp->cat), pp->cat, pp->pos );
   }
   else
   { /* it was not the first block */
      newblock = isamd_alloc_block(pp->is, firstpp->cat);
      pp->next = isamd_addr(newblock,firstpp->cat);
      isamd_buildlaterblock(pp);
      isamd_write_block(pp->is,pp->cat,pp->pos,pp->buf);
      pp->size = pp->offset = ISAMD_BLOCK_OFFSET_N;
      pp->next=0;
      pp->cat = firstpp->cat;
      pp->pos = isamd_block(firstpp->next);
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
   if (pp->is->method->debug >3)
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
      if (pp->is->method->debug >3)
         logf(LOG_LOG,"isamd:build: recoded into %s  (nk=%d)",
             hexdump(codebuff, c_ptr-codebuff,hexbuff), firstpp->numKeys+1);
   } /* block full */    
   
   /* write the data into pp, now we must have room */ 
   memcpy(&(pp->buf[pp->offset]),codebuff,codelen);
   pp->offset += codelen;
   pp->size += codelen;
   firstpp->numKeys++;
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
   
   if (is->method->debug >3)
      logf(LOG_LOG,"isamd_bld start: p=%d c=%d sz=%d maxsz=%d ",
         pp->pos, pp->cat, pp->size, 
         pp->is->method->filecat[pp->is->max_cat].bsize); 

   /* read first input */
   i_ptr = i_item;
   i_more = (*data->read_item)(data->clientData, &i_ptr, &i_mode); 
   if (i_more)
     assert( i_ptr-i_item == sizeof(i_key) );

   if (pp->is->method->debug >3)
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
      
      if ( (i_more) && (pp->is->method->debug >3) )
         logf(LOG_LOG,"isamd: build_fi start: m=%d %s",
            i_mode, hexdump(i_item,i_ptr-i_item,hexbuff) );
      
   } /* i_more */

   return save_both_pps( firstpp, pp );

} /* build_first_block */


/***************************************************************
 * Merging diffs 
 ***************************************************************/

 
static int merge ( ISAMD_PP *p_firstpp,   /* first pp of the chain */
                   ISAMD_PP *p_pp,        /* diff block */
                   struct it_key *p_key )
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
  
  if (*p_pp == *p_firstpp)
  { /* integrated diffs */
     diffidx=readpp->size;
     readpp->diffs = diffidx*2+0;
     readpp->diffbuf=readpp->buf;  /*? does this get freed right ???  */
     if (readpp->is->method->debug >3) 
         logf(LOG_LOG,"isamd_merge:local diffs at %d", diffidx);
  }
  else
  { /* separate diff block in *p_pp */
     killblk = readpp->diffs/2;
     diffidx = readpp->is->method->filecat[readpp->cat].bsize;
     readpp->diffbuf= xmalloc( diffidx); /* copy diffs to where read wants*/
     memcpy( &readpp->diffbuf, &((*p_pp)->buf[0]), diffidx);
     diffidx = ISAMD_BLOCK_OFFSET_N;
     if (readpp->is->method->debug >3) 
         logf(LOG_LOG,"isamd_merge:separate diffs at ix=%d", 
                 diffidx);
     if (readpp->is->method->debug >3) 
         logf(LOG_LOG,"isamd_merge: dbuf=%p (from %p) pp=%p", 
                  readpp->diffbuf, &((*p_pp)->buf[0]), (*p_pp) );
  }
  getDiffInfo(readpp,diffidx);

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

  if (killblk) 
  {
     isamd_release_block(readpp->is, isamd_type(killblk), isamd_block(killblk));
     /* we have the data, let the block go */
     if (readpp->is->method->debug >3) 
         logf(LOG_LOG,"isamd_merge: released diff block %d (%d:%d)",
              killblk, isamd_type(killblk), isamd_block(killblk) );
  }
  killblk=isamd_addr(readpp->pos, readpp->cat);
  isamd_release_block(readpp->is, isamd_type(killblk), isamd_block(killblk));
  if (readpp->is->method->debug >3) 
      logf(LOG_LOG,"isamd_merge: released old firstblock %d (%d:%d)",
            killblk, isamd_type(killblk), isamd_block(killblk) );
   
  /* set up the new blocks for simple writing */
  firstpp=pp=isamd_pp_open(readpp->is,isamd_addr(0, readpp->is->max_cat));
  firstpp->size = firstpp->offset = ISAMD_BLOCK_OFFSET_1;
  encoder_data = (*pp->is->method->code_start)(ISAMD_ENCODE);
  
  while (r_more)
  {
     if (readpp->is->method->debug >4) 
         logf(LOG_LOG,"isamd_merge: got key %d.%d",
           r_key.sysno, r_key.seqno );
     pp= append_main_item(firstpp, pp, &r_key, encoder_data);

     if ( isamd_addr(readpp->pos, readpp->cat) != killblk )
     {
        isamd_release_block(readpp->is, readpp->cat, readpp->pos);
        if (readpp->is->method->debug >3) 
            logf(LOG_LOG,"isamd_merge: released data block %d (%d:%d)",
                  killblk, isamd_type(killblk), isamd_block(killblk) );
        killblk=isamd_addr(readpp->pos, readpp->cat);
     }

     /* (try to) read next item */
     r_ptr= (char *) &r_key;
     r_more = isamd_read_item( readpp, &r_ptr);

  } /* while read */
  
  /* TODO: while pkey is an insert, and after last key inserted, append it */
  /* will prevent multiple merges on large insert runs */  

  /* set things up so that merge can continue */
  isamd_reduceblock(firstpp);

  if (firstpp!=pp) 
  { /* the last data block is of no interest any more */
    save_last_pp(pp);
    isamd_pp_close(pp);
  }
  
  *p_firstpp = firstpp; 
    
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
   if (is->method->debug >4)
      logf(LOG_LOG,"isamd_appd: Start ipos=%d=%d:%d d=%d=%d*2+%d",
        ipos, isamd_type(ipos), isamd_block(ipos),
        firstpp->diffs, firstpp->diffs/2, firstpp->diffs & 1);
   pp=read_diff_block(firstpp, &diffidx);
   encoder_data=(*is->method->code_start)(ISAMD_ENCODE);
   maxsize = is->method->filecat[pp->cat].bsize; 
   
   difflenidx = diffidx;
   diffidx+=sizeof(int);  /* difflen will be stored here */
   
   /* read first input */
   i_ptr = i_item;
   i_more = (*data->read_item)(data->clientData, &i_ptr, &i_mode); 

   if (is->method->debug >3)
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
      if (is->method->debug >3)
         logf(LOG_LOG,"isamd_appd: coded into %d: %s (nk=%d) (ix=%d)",
             codelen, hexdump(codebuff, codelen,hexbuff), 
             firstpp->numKeys,diffidx);

      if (diffidx + codelen > maxsize )
      { /* block full */
         if (is->method->debug >3)
            logf(LOG_LOG,"isamd_appd: block full (ix=%d mx=%d lix=%d)",
               diffidx, maxsize, difflenidx);
         if (is->method->debug >3)
            logf(LOG_LOG,"isamd_appd: block pp=%p buf=%p [%d]:%s",
               pp, pp->buf, difflenidx, hexdump(&pp->buf[difflenidx],8,0));
         merge_rc = merge (&firstpp, &pp, &i_key);
         if (merge_rc)
           return merge_rc;  /* merge handled them all ! */

          /* set things up so we can continue */
          pp = read_diff_block(firstpp, &diffidx);
          
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
      if ( (i_more) && (is->method->debug >3) )
         logf(LOG_LOG,"isamd_appd: got m=%d %s",
            i_mode, hexdump(i_item,i_ptr-i_item,hexbuff) );
   } /* more loop */

   /* clear the next difflen, if room for such */
   difflenidx = diffidx;
   while ( (difflenidx-diffidx<=sizeof(int)) && (difflenidx<maxsize))
     pp->buf[difflenidx++]='\0';

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

   return retval;
} /*  isamd_append */


/*
 * $Log: merge-d.c,v $
 * Revision 1.5  1999-07-23 13:58:52  heikki
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





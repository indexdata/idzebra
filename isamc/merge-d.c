/*
 * Copyright (c) 1996-1998, Index Data.
 * See the file LICENSE for details.
 * Heikki Levanto
 *
 * $Id: merge-d.c,v 1.4 1999-07-21 14:53:55 heikki Exp $
 * merge-d.c: merge routines for isamd
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
  return ( 0 != pp->next);
  /* Todo: This could be improved to check that there is a reasonable 
     amount of space in the block, or something... */
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
   assert ("too many diff sequences in the block");
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
      diffaddr=pp->diffs/2;
      isamd_read_block (pp->is, isamd_type(diffaddr), 
                                isamd_block(diffaddr), pp->diffbuf );
      diffidx= ISAMD_BLOCK_OFFSET_N; 
   }
   else
   { /* integrated block, just set the pointers */
     pp->diffbuf = pp->buf;
     diffidx = pp->size;  /* size is the beginning of diffs, diffidx the end*/
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
               assert("delete diff for nonexisting item");  
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



/*******************************************************************
 * Building main blocks (no diffs)
 *******************************************************************/

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
   int maxsize;
   
   char codebuff[128];
   char *c_ptr = codebuff;
   int codelen;
   
   char hexbuff[64];
   int newblock;
   int retval=0;
   
   firstpp=pp=isamd_pp_open(is, isamd_addr(0,is->max_cat));
   firstpp->size = firstpp->offset = ISAMD_BLOCK_OFFSET_1;
   encoder_data=(*is->method->code_start)(ISAMD_ENCODE);
   maxsize = is->method->filecat[is->max_cat].bsize; 
   
   if (is->method->debug >3)
      logf(LOG_LOG,"isamd_bld start: p=%d c=%d sz=%d maxsz=%d ",
         pp->pos, pp->cat, pp->size, maxsize);

   /* read first input */
   i_ptr = i_item;
   i_more = (*data->read_item)(data->clientData, &i_ptr, &i_mode); 
   if (i_more)
     assert( i_ptr-i_item == sizeof(i_key) );

   if (is->method->debug >3)
      logf(LOG_LOG,"isamd: build_fi start: m=%d %s",
         i_mode, hexdump(i_item,i_ptr-i_item,hexbuff) );

   while (i_more)
   { 
      if (i_mode!=0) 
      { /* ignore deletes here, should not happen */
      
         c_ptr=codebuff;
         i_ptr=i_item;
         (*is->method->code_item)(ISAMD_ENCODE, encoder_data, &c_ptr, &i_ptr);
         codelen = c_ptr - codebuff;
         assert ( (codelen<128) && (codelen>0));
         if (is->method->debug >3)
            logf(LOG_LOG,"isamd:build: coded into %s  (nk=%d)",
                hexdump(codebuff, c_ptr-codebuff,hexbuff), firstpp->numKeys+1);
     
         if (pp->offset + codelen > maxsize )
         { /* oops, block full - get a new one */
            if (firstpp==pp) 
            { /* special case: it was the first block. Save much later */
               if (0==firstpp->pos)
               { /* firstpp not allocated yet, do so now, */
                 /* to keep blocks in order. Don't save yet, though */
                  firstpp->pos = isamd_alloc_block(is, firstpp->cat);
               }
               newblock = isamd_alloc_block(is, firstpp->cat);
               firstpp->next = isamd_addr(newblock,firstpp->cat);
                 /* keep the largest category */
               pp=isamd_pp_open(is,isamd_addr(0,firstpp->cat));/*don't load*/
               pp->pos=newblock; 
               pp->size = pp->offset = ISAMD_BLOCK_OFFSET_N; 
               pp->next=0;
               if (is->method->debug >3)
                  logf(LOG_LOG,"isamd_build: Alloc2 f=%d (%d:%d) n=%d(%d:%d)",
                     isamd_addr(firstpp->pos,firstpp->cat), 
                     firstpp->cat, firstpp->pos,
                     isamd_addr(pp->pos,pp->cat), pp->cat, pp->pos );
            }
            else
            { /* it was not the first block */
               newblock = isamd_alloc_block(is, firstpp->cat);
               pp->next = isamd_addr(newblock,firstpp->cat);
               isamd_buildlaterblock(pp);
               isamd_write_block(is,pp->cat,pp->pos,pp->buf);
               pp->size = pp->offset = ISAMD_BLOCK_OFFSET_N;
               pp->next=0;
               pp->cat = firstpp->cat;
               pp->pos = isamd_block(firstpp->next);
            }
            /* reset encoging and code again */
            (*is->method->code_reset)(encoder_data);
            c_ptr=codebuff;
            i_ptr=i_item;
            (*is->method->code_item)(ISAMD_ENCODE, encoder_data, &c_ptr, &i_ptr);
            codelen = c_ptr - codebuff;
            assert ( (codelen<128) && (codelen>0));
            if (is->method->debug >3)
               logf(LOG_LOG,"isamd:build: recoded into %s  (nk=%d)",
                   hexdump(codebuff, c_ptr-codebuff,hexbuff), firstpp->numKeys+1);
         } /* block full */    
         
         /* write the data into pp, now we have room */ 
         memcpy(&(pp->buf[pp->offset]),codebuff,codelen);
         pp->offset += codelen;
         pp->size += codelen;
         firstpp->numKeys++;
      } /* not delete */
      
      /* (try to) read the next item */
      i_ptr = i_item;
      i_more = (*data->read_item)(data->clientData, &i_ptr, &i_mode); 
      
      if ( (i_more) && (is->method->debug >3) )
         logf(LOG_LOG,"isamd: build_fi start: m=%d %s",
            i_mode, hexdump(i_item,i_ptr-i_item,hexbuff) );
      
      
   } /* i_more */

   /* order of things: Better to save firstpp first, if there are just two */
   /* blocks, but last if there are blocks in between, as these have already */
   /* been saved... optimise later */

   /* save the first block */
   isamd_reduceblock(firstpp);
   isamd_buildfirstblock(firstpp);
   isamd_write_block(is,firstpp->cat,firstpp->pos,firstpp->buf);   
   retval = isamd_addr(firstpp->pos,firstpp->cat);

   if (firstpp!=pp){  /* and the last one */
      pp->next = 0;/* just to be sure */
      isamd_buildlaterblock(pp);
      isamd_write_block(is,pp->cat,pp->pos,pp->buf);
      isamd_pp_close(pp);
   }

   isamd_pp_close(firstpp);
   
   return retval;
} /* build_first_block */


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
   int retval=ipos;  /* by default we do not change the firstblock addr */
   int diffidx=0;
   int maxsize=0;
   int difflenidx;
   char codebuff[128];
   char *c_ptr = codebuff;
   int codelen;

   pp=firstpp=isamd_pp_open(is, ipos);

   /* TODO: Turn these ifs around! Check first diffs==0, and create new */
   /* according to separateDiffBlock. if !=0, read according to its type */
   /* bit. Much more robust this way around! */
   
   if (separateDiffBlock(firstpp))
   { /* multi-block item, get the diff block */
     if (firstpp->diffs==0) 
     { /* allocate first diff block */
       pp=isamd_pp_open(is,isamd_addr(0,firstpp->cat));
       pp->pos = isamd_alloc_block(pp->is, pp->cat);
       firstpp->diffs = pp->pos*2 +1;
       diffidx = pp->size = pp->offset = ISAMD_BLOCK_OFFSET_N;
       if (is->method->debug >3) 
           logf(LOG_LOG,"isamd_appd: alloc diff  (%d) %d %d:%d ix=%d",
                 firstpp->diffs,
                 isamd_addr(pp->pos,pp->cat), pp->cat, pp->pos, 
                 diffidx);
     }
     else
     { /* find pointers within the existing block */
        pp=isamd_pp_open(is, firstpp->diffs/2);
        diffidx = pp->offset= pp->size;
        if (is->method->debug >3) 
           logf(LOG_LOG,"isamd_appd: load diff (%d) %d (%d:%d) ix=%d",
                 firstpp->diffs,
                 isamd_addr(pp->pos,pp->cat), pp->cat, pp->pos, 
                 diffidx);
     }
   } 
   else
   { /* single-block item, get idx right */
     diffidx= pp->diffs/2;
     if (is->method->debug >3) 
        logf(LOG_LOG,"isamd_appd: diffs in head %d (%d:%d) ix=%d sz=%d",
                 isamd_addr(pp->pos,pp->cat), pp->cat, pp->pos, 
                 diffidx, pp->size);
     if (0==diffidx)
     { /* no diffs yet, make them */
        diffidx = pp->size;
        pp->diffs = diffidx *2 +0;  
     }
   }
   
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
         logf(LOG_LOG,"isamd_appd: block full (ix=%d mx=%d)",
            diffidx, maxsize);
         return 0;         /*!*/ /* do something about it!!! */
      } /* block full */
      
      /* save the diff */ 
      memcpy(&(pp->buf[diffidx]),codebuff,codelen);
      diffidx += codelen;
      if (i_mode)
        firstpp->numKeys++; /* insert diff */
      else
        firstpp->numKeys--; /* delete diff */ 
      memcpy(&(pp->buf[difflenidx]),&diffidx,sizeof(diffidx));
      if (firstpp==pp)
        firstpp->diffs =diffidx*2+0;
      else
        pp->size =diffidx;
      
      /* try to read the next input */
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
   
   /* ok, save the block(s) */
   if (firstpp != pp) 
   { /* save the diff block */
      pp->next = 0;  /* just to be sure */
      isamd_buildlaterblock(pp);
      isamd_write_block(is,pp->cat,pp->pos,pp->buf);
      isamd_pp_close(pp);
   }

   isamd_buildfirstblock(firstpp);
   isamd_write_block(is,firstpp->cat,firstpp->pos,firstpp->buf);   
   retval = isamd_addr(firstpp->pos,firstpp->cat);
   isamd_pp_close(firstpp);

   return retval;
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
 * Revision 1.4  1999-07-21 14:53:55  heikki
 * isamd read and write functions work, except when block full
 * Merge missing still. Need to split some functions
 *
 * Revision 1.1  1999/07/14 13:14:47  heikki
 * Created empty
 *
 *
 */





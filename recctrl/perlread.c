/* $Id: perlread.c,v 1.7 2003-02-28 16:20:10 pop Exp $
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
along with Zebra; see the file LICENSE.zebra.  If not, write to the
Free Software Foundation, 59 Temple Place - Suite 330, Boston, MA
02111-1307, USA.
*/

#if HAVE_PERL
#include "perlread.h"
#include "EXTERN.h"
#include "perl.h"
#include "XSUB.h"

#include <stdio.h>
#include <string.h>
#include <ctype.h>

#include <yaz/tpath.h>
#include <zebrautl.h>
#include <dfa.h>
#include "grsread.h"


#define GRS_PERL_MODULE_NAME_MAXLEN 255

/* Context information for the filter */
struct perl_context {
  PerlInterpreter *perli;
  PerlInterpreter *origi;
  int perli_ready;
  char filterClass[GRS_PERL_MODULE_NAME_MAXLEN];
  SV *filterRef;

  int (*readf)(void *, char *, size_t);
  off_t (*seekf)(void *, off_t);
  off_t (*tellf)(void *);
  void (*endf)(void *, off_t);

  void *fh;
  data1_handle dh;
  NMEM mem;
  data1_node *res;
};

/* Constructor call for the filter object */
void Filter_create (struct perl_context *context) 
{
  dSP;
  SV *msv;

  PUSHMARK(SP) ;
  XPUSHs(sv_2mortal(newSVpv(context->filterClass, 
			    strlen(context->filterClass)))) ;
  msv = sv_newmortal();
  sv_setref_pv(msv, "_p_perl_context", (void*)context);
  XPUSHs(msv) ;
  PUTBACK ;
  call_method("new", G_EVAL);

  SPAGAIN ;
  context->filterRef = POPs;
  PUTBACK ;
}

/*
 Execute the process call on the filter. This is a bit dirty. 
 The perl code is going to get dh and nmem from the context trough callbacks,
 then call readf, to get the stream, and then set the res (d1 node)
 in the context. However, it's safer, to let swig do as much of wrapping
 as possible.
 */
int Filter_process (struct perl_context *context)

{

  int res;

  dSP;

  ENTER;
  SAVETMPS;

  PUSHMARK(SP) ;
  XPUSHs(context->filterRef);
  PUTBACK ;
  call_method("_process", 0);
  SPAGAIN ;
  res = POPi;
  PUTBACK ;

  FREETMPS;
  LEAVE;

  return (res);
}

/*
 This one is called to transfer the results of a readf. It's going to create 
 a temporary variable there...

 So the call stack is something like:


 ->Filter_process(context)                            [C]
   -> _process($context)                              [Perl]
    -> grs_perl_get_dh($context)                      [Perl]
      -> grs_perl_get_dh(context)                     [C]
    -> grs_perl_get_mem($context)                     [Perl]
      -> grs_perl_get_mem(context)                    [C]
    -> process()                                      [Perl]
      ...
      -> grs_perl_readf($context,$len)                [Perl]
        -> grs_perl_readf(context, len)               [C]
           ->(*context->readf)(context->fh, buf, len) [C]
        -> Filter_store_buff(context, buf, r)         [C]
           -> _store_buff($buff)                      [Perl]
        [... returns buff and length ...]
      ...
      [... returns d1 node ...]
    -> grs_perl_set_res($context, $node)              [Perl]
      -> grs_perl_set_res(context, node)              [C]

 [... The result is in context->res ...] 

  Dirty, isn't it? It may become nicer, if I'll have some more time to work on
  it. However, these changes are not going to hurt the filter api, as
  Filter.pm, which is inherited into all specific filter implementations
  can hide this whole compexity behind.

*/
void Filter_store_buff (struct perl_context *context, char *buff, size_t len) {
  dSP;

  ENTER;
  SAVETMPS;

  PUSHMARK(SP) ;
  XPUSHs(context->filterRef);
  XPUSHs(sv_2mortal(newSVpv(buff, len)));  
  PUTBACK ;
  call_method("_store_buff", 0);
  SPAGAIN ;
  PUTBACK ;

  FREETMPS;
  LEAVE;
}
/*  The "file" manipulation function wrappers */
int grs_perl_readf(struct perl_context *context, size_t len) {
  int r;
  char *buf = (char *) xmalloc (len+1);
  r = (*context->readf)(context->fh, buf, len);
  if (r > 0) Filter_store_buff (context, buf, r);
  xfree (buf);
  return (r);
}

int grs_perl_readline(struct perl_context *context) {
  int r;
  char *buf = (char *) xmalloc (4096);
  char *p = buf;
  
  while ((r = (*context->readf)(context->fh,p,1)) && (p-buf < 4095)) {
    p++;
    if (*(p-1) == 10) break;
  }

  *p = 0;

  if (p != buf) Filter_store_buff (context, buf, p - buf);
  xfree (buf);
  return (p - buf);
}

char grs_perl_getc(struct perl_context *context) {
  int r;
  char *p;
  if (r = (*context->readf)(context->fh,p,1)) {
    return (*p);
  } else {
    return (0);
  }
}

off_t grs_perl_seekf(struct perl_context *context, off_t offset) {
  return ((*context->seekf)(context->fh, offset));
}

off_t grs_perl_tellf(struct perl_context *context) {
  return ((*context->tellf)(context->fh));
}

void grs_perl_endf(struct perl_context *context, off_t offset) {
  (*context->endf)(context->fh, offset);
}

/* Get pointers from the context. Easyer to wrap this by SWIG */
data1_handle *grs_perl_get_dh(struct perl_context *context) {
  return(&context->dh);
}

NMEM *grs_perl_get_mem(struct perl_context *context) {
  return(&context->mem);
}

/* Set the result in the context */
void grs_perl_set_res(struct perl_context *context, data1_node *n) {
  context->res = n;
}

/* The filter handlers (init, destroy, read) */
static void *grs_init_perl(void)
{
  struct perl_context *context = 
    (struct perl_context *) xmalloc (sizeof(*context));

  /* If there is an interpreter (context) running, - we are calling 
     indexing and retrieval from the perl API - we don't create a new one. */
  context->origi = PERL_GET_CONTEXT;
  if (context->origi == NULL) {
    context->perli = perl_alloc();
    PERL_SET_CONTEXT(context->perli);
    logf (LOG_LOG, "Initializing new perl interpreter context (%p)",context->perli);
  } else {
    logf (LOG_LOG, "Using existing perl interpreter context (%p)",context->origi);
  }
  context->perli_ready = 0;
  strcpy(context->filterClass, "");
  return (context);
}

void grs_destroy_perl(void *clientData)
{
  struct perl_context *context = (struct perl_context *) clientData;

  logf (LOG_LOG, "Destroying perl interpreter context");
  if (context->perli_ready) {
    /*
    FREETMPS;
    LEAVE;
    */
    if (context->origi == NULL)  perl_destruct(context->perli);
   }
  if (context->origi == NULL) perl_free(context->perli);
  xfree (context);
}

static data1_node *grs_read_perl (struct grs_read_info *p)
{
  struct perl_context *context = (struct perl_context *) p->clientData;
  char *filterClass = p->type;

  /* The "file" manipulation function wrappers */
  context->readf = p->readf;
  context->seekf = p->seekf;
  context->tellf = p->tellf;
  context->endf  = p->endf;

  /* The "file", data1 and NMEM handles */
  context->fh  = p->fh;
  context->dh  = p->dh;
  context->mem = p->mem;

  /* If the class was not interpreted before... */
  /* This is not too efficient, when indexing with many different filters... */
  if (strcmp(context->filterClass,filterClass)) {

    char modarg[GRS_PERL_MODULE_NAME_MAXLEN + 2];
    char initarg[GRS_PERL_MODULE_NAME_MAXLEN + 2];
    char *arglist[6] = { "", "-I", "", "-M", "-e", "" };

    if (context->perli_ready) {
      /*
      FREETMPS;
      LEAVE;
      */
      if (context->origi == NULL) perl_destruct(context->perli);
    }
    if (context->origi == NULL) perl_construct(context->perli);
    
    /*
    ENTER;
    SAVETMPS;
    */
    context->perli_ready = 1;

    /* parse, and run the init call */
    if (context->origi == NULL) {
      logf (LOG_LOG, "Interpreting filter class:%s", filterClass);

      arglist[2] = (char *) data1_get_tabpath(p->dh);
      sprintf(modarg,"-M%s",filterClass);
      arglist[3] = (char *) &modarg;
      sprintf(initarg,"%s->init;",filterClass);
      arglist[5] = (char *) &initarg;

      perl_parse(context->perli, PERL_XS_INIT, 6, arglist, NULL);
      perl_run(context->perli);
    } 

    strcpy(context->filterClass, filterClass);

    /* create the filter object as a filterClass blessed reference */
    Filter_create(context);
  }

  /* Wow... if calling with individual update_record calls from perl,
     the filter object reference may go out of scope... */
  if (!sv_isa(context->filterRef, context->filterClass)) {
    Filter_create(context);
    logf (LOG_DEBUG,"Filter recreated");
  }

  if (!SvTRUE(context->filterRef)) {
    logf (LOG_WARN,"Failed to initialize perl filter %s",context->filterClass);
    return (0);
  }

  /* call the process method */
  Filter_process(context);

  /* return the created data1 node */
  return (context->res);
}

static struct recTypeGrs perl_type = {
    "perl",
    grs_init_perl,
    grs_destroy_perl,
    grs_read_perl
};

RecTypeGrs recTypeGrs_perl = &perl_type;

/* HAVE_PERL */
#endif

/* $Id: kinput.c,v 1.61 2004-08-06 13:14:46 adam Exp $
   Copyright (C) 1995,1996,1997,1998,1999,2000,2001,2002,2003,2004
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
 
#include <fcntl.h>
#ifdef WIN32
#include <io.h>
#else
#include <unistd.h>
#endif
#include <stdlib.h>
#include <string.h>
#include <stdio.h>
#include <assert.h>

#include "index.h"

#define KEY_SIZE (1+sizeof(struct it_key))
#define INP_NAME_MAX 768
#define INP_BUF_START 60000
#define INP_BUF_ADD  400000


struct key_file {
    int   no;            /* file no */
    off_t offset;        /* file offset */
    unsigned char *buf;  /* buffer block */
    size_t buf_size;     /* number of read bytes in block */
    size_t chunk;        /* number of bytes allocated */
    size_t buf_ptr;      /* current position in buffer */
    char *prev_name;     /* last word read */
#if IT_KEY_NEW
    void *decode_handle;
#else
    int   sysno;         /* last sysno */
    int   seqno;         /* last seqno */
#endif
    off_t length;        /* length of file */
                         /* handler invoked in each read */
    void (*readHandler)(struct key_file *keyp, void *rinfo);
    void *readInfo;
    Res res;
};

void getFnameTmp (Res res, char *fname, int no)
{
    const char *pre;
    
    pre = res_get_def (res, "keyTmpDir", ".");
    sprintf (fname, "%s/key%d.tmp", pre, no);
}

void extract_get_fname_tmp (ZebraHandle zh, char *fname, int no)
{
    const char *pre;
    
    pre = res_get_def (zh->res, "keyTmpDir", ".");
    sprintf (fname, "%s/key%d.tmp", pre, no);
}

void key_file_chunk_read (struct key_file *f)
{
    int nr = 0, r = 0, fd;
    char fname[1024];
    getFnameTmp (f->res, fname, f->no);
    fd = open (fname, O_BINARY|O_RDONLY);

    f->buf_ptr = 0;
    f->buf_size = 0;
    if (fd == -1)
    {
        logf (LOG_WARN|LOG_ERRNO, "cannot open %s", fname);
	return ;
    }
    if (!f->length)
    {
        if ((f->length = lseek (fd, 0L, SEEK_END)) == (off_t) -1)
        {
            logf (LOG_WARN|LOG_ERRNO, "cannot seek %s", fname);
	    close (fd);
	    return ;
        }
    }
    if (lseek (fd, f->offset, SEEK_SET) == -1)
    {
        logf (LOG_WARN|LOG_ERRNO, "cannot seek %s", fname);
	close(fd);
	return ;
    }
    while (f->chunk - nr > 0)
    {
        r = read (fd, f->buf + nr, f->chunk - nr);
        if (r <= 0)
            break;
        nr += r;
    }
    if (r == -1)
    {
        logf (LOG_WARN|LOG_ERRNO, "read of %s", fname);
	close (fd);
	return;
    }
    f->buf_size = nr;
    if (f->readHandler)
        (*f->readHandler)(f, f->readInfo);
    close (fd);
}

void key_file_destroy (struct key_file *f)
{
#if IT_KEY_NEW
    iscz1_stop(f->decode_handle);
#endif
    xfree (f->buf);
    xfree (f->prev_name);
    xfree (f);
}

struct key_file *key_file_init (int no, int chunk, Res res)
{
    struct key_file *f;

    f = (struct key_file *) xmalloc (sizeof(*f));
    f->res = res;
#if IT_KEY_NEW
    f->decode_handle = iscz1_start();
#else
    f->sysno = 0;
    f->seqno = 0;
#endif
    f->no = no;
    f->chunk = chunk;
    f->offset = 0;
    f->length = 0;
    f->readHandler = NULL;
    f->buf = (unsigned char *) xmalloc (f->chunk);
    f->prev_name = (char *) xmalloc (INP_NAME_MAX);
    *f->prev_name = '\0';
    key_file_chunk_read (f);
    return f;
}

int key_file_getc (struct key_file *f)
{
    if (f->buf_ptr < f->buf_size)
        return f->buf[(f->buf_ptr)++];
    if (f->buf_size < f->chunk)
        return EOF;
    f->offset += f->buf_size;
    key_file_chunk_read (f);
    if (f->buf_ptr < f->buf_size)
        return f->buf[(f->buf_ptr)++];
    else
        return EOF;
}

int key_file_decode (struct key_file *f)
{
    int c, d;

    c = key_file_getc (f);
    switch (c & 192) 
    {
    case 0:
        d = c;
        break;
    case 64:
        d = ((c&63) << 8) + (key_file_getc (f) & 0xff);
        break;
    case 128:
        d = ((c&63) << 8) + (key_file_getc (f) & 0xff);
        d = (d << 8) + (key_file_getc (f) & 0xff);
        break;
    default: /* 192 */
        d = ((c&63) << 8) + (key_file_getc (f) & 0xff);
        d = (d << 8) + (key_file_getc (f) & 0xff);
        d = (d << 8) + (key_file_getc (f) & 0xff);
        break;
    }
    return d;
}

int key_file_read (struct key_file *f, char *key)
{
    int i, c;
    char srcbuf[128];
#if IT_KEY_NEW
    const char *src = srcbuf;
    char *dst;
    int j;
#else
    struct it_key itkey;
    int d;
#endif

    c = key_file_getc (f);
    if (c == 0)
    {
        strcpy (key, f->prev_name);
        i = 1+strlen (key);
    }
    else if (c == EOF)
        return 0;
    else
    {
        i = 0;
        key[i++] = c;
        while ((key[i++] = key_file_getc (f)))
            ;
        strcpy (f->prev_name, key);
#if IT_KEY_NEW
	iscz1_reset(f->decode_handle);
#endif
    }
#if IT_KEY_NEW
    c = key_file_getc(f); /* length +  insert/delete combined */
    key[i++] = c & 128;
    c = c & 127;
    for (j = 0; j < c; j++)
	srcbuf[j] = key_file_getc(f);
    dst = key + i;
    iscz1_decode(f->decode_handle, &dst, &src);
    return i + sizeof(struct it_key);
#else
    d = key_file_decode (f);
    key[i++] = d & 1;
    d = d >> 1;
    itkey.sysno = d + f->sysno;
    if (d) 
    {
        f->sysno = itkey.sysno;
        f->seqno = 0;
    }
    d = key_file_decode (f);
    itkey.seqno = d + f->seqno;
    f->seqno = itkey.seqno;
    memcpy (key + i, &itkey, sizeof(struct it_key));
    return i + sizeof (struct it_key);
#endif
}

struct heap_info {
    struct {
        struct key_file **file;
        char   **buf;
    } info;
    int    heapnum;
    int    *ptr;
    int    (*cmp)(const void *p1, const void *p2);
    struct zebra_register *reg;
    ZebraHandle zh; /* only used for raw reading that bypasses the heaps */
    int no_diffs;
    int no_updates;
    int no_deletions;
    int no_insertions;
    int no_iterations;
};

static struct heap_info *key_heap_malloc()
{  /* malloc and clear it */
    struct heap_info *hi;
    hi = (struct heap_info *) xmalloc (sizeof(*hi));
    hi->info.file = 0;
    hi->info.buf = 0;
    hi->heapnum = 0;
    hi->ptr = 0;
    hi->zh=0;
    hi->no_diffs = 0;
    hi->no_diffs = 0;
    hi->no_updates = 0;
    hi->no_deletions = 0;
    hi->no_insertions = 0;
    hi->no_iterations = 0;
    return hi;
}

struct heap_info *key_heap_init (int nkeys,
                                 int (*cmp)(const void *p1, const void *p2))
{
    struct heap_info *hi;
    int i;

    hi = key_heap_malloc();
    hi->info.file = (struct key_file **)
        xmalloc (sizeof(*hi->info.file) * (1+nkeys));
    hi->info.buf = (char **) xmalloc (sizeof(*hi->info.buf) * (1+nkeys));
    hi->ptr = (int *) xmalloc (sizeof(*hi->ptr) * (1+nkeys));
    hi->cmp = cmp;
    for (i = 0; i<= nkeys; i++)
    {
        hi->ptr[i] = i;
        hi->info.buf[i] = (char *) xmalloc (INP_NAME_MAX);
    }
    return hi;
}

struct heap_info *key_heap_init_buff ( ZebraHandle zh,
                                 int (*cmp)(const void *p1, const void *p2))
{
    struct heap_info *hi=key_heap_malloc();
    hi->cmp=cmp;
    hi->zh=zh;
    return hi;
}

void key_heap_destroy (struct heap_info *hi, int nkeys)
{
    int i;
    yaz_log (LOG_DEBUG, "key_heap_destroy");
    yaz_log (LOG_DEBUG, "key_heap_destroy nk=%d",nkeys);
    if (!hi->zh)
        for (i = 0; i<=nkeys; i++)
            xfree (hi->info.buf[i]);
    
    xfree (hi->info.buf);
    xfree (hi->ptr);
    xfree (hi->info.file);
    xfree (hi);
}

static void key_heap_swap (struct heap_info *hi, int i1, int i2)
{
    int swap;

    swap = hi->ptr[i1];
    hi->ptr[i1] = hi->ptr[i2];
    hi->ptr[i2] = swap;
}


static void key_heap_delete (struct heap_info *hi)
{
    int cur = 1, child = 2;

    assert (hi->heapnum > 0);

    key_heap_swap (hi, 1, hi->heapnum);
    hi->heapnum--;
    while (child <= hi->heapnum) {
        if (child < hi->heapnum &&
            (*hi->cmp)(&hi->info.buf[hi->ptr[child]],
                       &hi->info.buf[hi->ptr[child+1]]) > 0)
            child++;
        if ((*hi->cmp)(&hi->info.buf[hi->ptr[cur]],
                       &hi->info.buf[hi->ptr[child]]) > 0)
        {            
            key_heap_swap (hi, cur, child);
            cur = child;
            child = 2*cur;
        }
        else
            break;
    }
}

static void key_heap_insert (struct heap_info *hi, const char *buf, int nbytes,
                             struct key_file *kf)
{
    int cur, parent;

    cur = ++(hi->heapnum);
    memcpy (hi->info.buf[hi->ptr[cur]], buf, nbytes);
    hi->info.file[hi->ptr[cur]] = kf;

    parent = cur/2;
    while (parent && (*hi->cmp)(&hi->info.buf[hi->ptr[parent]],
                                &hi->info.buf[hi->ptr[cur]]) > 0)
    {
        key_heap_swap (hi, cur, parent);
        cur = parent;
        parent = cur/2;
    }
}

static int heap_read_one_raw (struct heap_info *hi, char *name, char *key)
{
    ZebraHandle zh=hi->zh;
    size_t ptr_i = zh->reg->ptr_i;
    char *cp;
    if (!ptr_i)
        return 0;
    --(zh->reg->ptr_i);
    cp=(zh->reg->key_buf)[zh->reg->ptr_top - ptr_i];
    logf (LOG_DEBUG, " raw: i=%ld top=%ld cp=%p", (long) ptr_i,
	  (long) zh->reg->ptr_top, cp);
    strcpy(name, cp);
    memcpy(key, cp+strlen(name)+1, KEY_SIZE);
    hi->no_iterations++;
    return 1;
}

static int heap_read_one (struct heap_info *hi, char *name, char *key)
{
    int n, r;
    char rbuf[INP_NAME_MAX];
    struct key_file *kf;

    if (hi->zh) /* bypass the heap stuff, we have a readymade buffer */
        return heap_read_one_raw(hi, name, key);

    if (!hi->heapnum)
        return 0;
    n = hi->ptr[1];
    strcpy (name, hi->info.buf[n]);
    kf = hi->info.file[n];
    r = strlen(name);
    memcpy (key, hi->info.buf[n] + r+1, KEY_SIZE);
    key_heap_delete (hi);
    if ((r = key_file_read (kf, rbuf)))
        key_heap_insert (hi, rbuf, r, kf);
    hi->no_iterations++;
    return 1;
}

#define PR_KEY 0

#if PR_KEY
static void pkey(const char *b, int mode)
{
    struct it_key *key = (struct it_key *) b;
    printf ("%c %d:%d\n", mode + 48, key->sysno, key->seqno);
}
#endif

struct heap_cread_info {
    char prev_name[INP_NAME_MAX];
    char cur_name[INP_NAME_MAX];
    char *key;
    char *key_1, *key_2;
    int mode_1, mode_2;
    int sz_1, sz_2;
    struct heap_info *hi;
    int first_in_list;
    int more;
    int ret;
};

static int heap_cread_item (void *vp, char **dst, int *insertMode);

int heap_cread_item2 (void *vp, char **dst, int *insertMode)
{
    struct heap_cread_info *p = (struct heap_cread_info *) vp;
    int level = 0;

    if (p->ret == 0)    /* lookahead was 0?. Return that in read next round */
    {
        p->ret = -1;
        return 0;
    }
    else if (p->ret == -1) /* Must read new item ? */
    {
        char *dst_1 = p->key_1;
        p->ret = heap_cread_item(vp, &dst_1, &p->mode_1);
        p->sz_1 = dst_1 - p->key_1;
    }
    else
    {        /* lookahead in 2 . Now in 1. */
        p->sz_1 = p->sz_2;
        p->mode_1 = p->mode_2;
        memcpy (p->key_1, p->key_2, p->sz_2);
    }
    if (p->mode_1)
        level = 1;     /* insert */
    else
        level = -1;    /* delete */
    while(1)
    {
        char *dst_2 = p->key_2;
        p->ret = heap_cread_item(vp, &dst_2, &p->mode_2);
        if (!p->ret)
        {
            if (level)
                break;
            p->ret = -1;
            return 0;
        }
        p->sz_2 = dst_2 - p->key_2;
        if (p->sz_1 == p->sz_2 && memcmp(p->key_1, p->key_2, p->sz_1) == 0)
        {
            if (p->mode_2) /* adjust level according to deletes/inserts */
                level++;
            else
                level--;
        }
        else
        {
            if (level)
                break;
            /* all the same. new round .. */
            p->sz_1 = p->sz_2;
            p->mode_1 = p->mode_2;
            memcpy (p->key_1, p->key_2, p->sz_1);
            if (p->mode_1)
                level = 1;     /* insert */
            else
                level = -1;    /* delete */
        }
    }
    /* outcome is insert (1) or delete (0) depending on final level */
    if (level > 0)
        *insertMode = 1;
    else
        *insertMode = 0;
    memcpy (*dst, p->key_1, p->sz_1);
#if PR_KEY
    printf ("top: ");
    pkey(*dst, *insertMode); fflush(stdout);
#endif
    (*dst) += p->sz_1;
    return 1;
}
      
int heap_cread_item (void *vp, char **dst, int *insertMode)
{
    struct heap_cread_info *p = (struct heap_cread_info *) vp;
    struct heap_info *hi = p->hi;

    if (p->first_in_list)
    {
        *insertMode = p->key[0];
        memcpy (*dst, p->key+1, sizeof(struct it_key));
#if PR_KEY
        printf ("sub1: ");
        pkey(*dst, *insertMode);
#endif
        (*dst) += sizeof(struct it_key);
        p->first_in_list = 0;
        return 1;
    }
    strcpy (p->prev_name, p->cur_name);
    if (!(p->more = heap_read_one (hi, p->cur_name, p->key)))
        return 0;
    if (*p->cur_name && strcmp (p->cur_name, p->prev_name))
    {
        p->first_in_list = 1;
        return 0;
    }
    *insertMode = p->key[0];
    memcpy (*dst, p->key+1, sizeof(struct it_key));
#if PR_KEY
    printf ("sub2: ");
    pkey(*dst, *insertMode);
#endif
    (*dst) += sizeof(struct it_key);
    return 1;
}

int heap_inpc (struct heap_info *hi)
{
    struct heap_cread_info hci;
    ISAMC_I *isamc_i = (ISAMC_I *) xmalloc (sizeof(*isamc_i));

    hci.key = (char *) xmalloc (KEY_SIZE);
    hci.key_1 = (char *) xmalloc (KEY_SIZE);
    hci.key_2 = (char *) xmalloc (KEY_SIZE);
    hci.ret = -1;
    hci.first_in_list = 1;
    hci.hi = hi;
    hci.more = heap_read_one (hi, hci.cur_name, hci.key);

    isamc_i->clientData = &hci;
    isamc_i->read_item = heap_cread_item2;

    while (hci.more)
    {
        char this_name[INP_NAME_MAX];
        ISAMC_P isamc_p, isamc_p2;
        char *dict_info;

        strcpy (this_name, hci.cur_name);
	assert (hci.cur_name[1]);
        hi->no_diffs++;
        if ((dict_info = dict_lookup (hi->reg->dict, hci.cur_name)))
        {
            memcpy (&isamc_p, dict_info+1, sizeof(ISAMC_P));
            isamc_p2 = isc_merge (hi->reg->isamc, isamc_p, isamc_i);
            if (!isamc_p2)
            {
                hi->no_deletions++;
                if (!dict_delete (hi->reg->dict, this_name))
                    abort();
            }
            else 
            {
                hi->no_updates++;
                if (isamc_p2 != isamc_p)
                    dict_insert (hi->reg->dict, this_name,
                                 sizeof(ISAMC_P), &isamc_p2);
            }
        } 
        else
        {
            isamc_p = isc_merge (hi->reg->isamc, 0, isamc_i);
            hi->no_insertions++;
            dict_insert (hi->reg->dict, this_name, sizeof(ISAMC_P), &isamc_p);
        }
    }
    xfree (isamc_i);
    xfree (hci.key);
    xfree (hci.key_1);
    xfree (hci.key_2);
    return 0;
} 

#if 0
/* for debugging only */
static void print_dict_item (ZebraMaps zm, const char *s)
{
    int reg_type = s[1];
    char keybuf[IT_MAX_WORD+1];
    char *to = keybuf;
    const char *from = s + 2;

    while (*from)
    {
        const char *res = zebra_maps_output (zm, reg_type, &from);
        if (!res)
            *to++ = *from++;
        else
            while (*res)
                *to++ = *res++;
    }
    *to = '\0';
    yaz_log (LOG_LOG, "%s", keybuf);
}
#endif

int heap_inpb (struct heap_info *hi)
{
    struct heap_cread_info hci;
    ISAMC_I *isamc_i = (ISAMC_I *) xmalloc (sizeof(*isamc_i));

    hci.key = (char *) xmalloc (KEY_SIZE);
    hci.key_1 = (char *) xmalloc (KEY_SIZE);
    hci.key_2 = (char *) xmalloc (KEY_SIZE);
    hci.ret = -1;
    hci.first_in_list = 1;
    hci.hi = hi;
    hci.more = heap_read_one (hi, hci.cur_name, hci.key);

    isamc_i->clientData = &hci;
    isamc_i->read_item = heap_cread_item2;

    while (hci.more)
    {
        char this_name[INP_NAME_MAX];
        ISAMC_P isamc_p, isamc_p2;
        char *dict_info;

        strcpy (this_name, hci.cur_name);
	assert (hci.cur_name[1]);
        hi->no_diffs++;

#if 0
        print_dict_item (hi->reg->zebra_maps, hci.cur_name);
#endif
        if ((dict_info = dict_lookup (hi->reg->dict, hci.cur_name)))
        {
            memcpy (&isamc_p, dict_info+1, sizeof(ISAMC_P));
            isamc_p2 = isamb_merge (hi->reg->isamb, isamc_p, isamc_i);
            if (!isamc_p2)
            {
                hi->no_deletions++;
                if (!dict_delete (hi->reg->dict, this_name))
                    abort();
            }
            else 
            {
                hi->no_updates++;
                if (isamc_p2 != isamc_p)
                    dict_insert (hi->reg->dict, this_name,
                                 sizeof(ISAMC_P), &isamc_p2);
            }
        } 
        else
        {
            isamc_p = isamb_merge (hi->reg->isamb, 0, isamc_i);
            hi->no_insertions++;
            dict_insert (hi->reg->dict, this_name, sizeof(ISAMC_P), &isamc_p);
        }
    }
    xfree (isamc_i);
    xfree (hci.key);
    xfree (hci.key_1);
    xfree (hci.key_2);
    return 0;
} 

int heap_inps (struct heap_info *hi)
{
    struct heap_cread_info hci;
    ISAMS_I isams_i = (ISAMS_I) xmalloc (sizeof(*isams_i));

    hci.key = (char *) xmalloc (KEY_SIZE);
    hci.key_1 = (char *) xmalloc (KEY_SIZE);
    hci.key_2 = (char *) xmalloc (KEY_SIZE);
    hci.first_in_list = 1;
    hci.ret = -1;
    hci.hi = hi;
    hci.more = heap_read_one (hi, hci.cur_name, hci.key);

    isams_i->clientData = &hci;
    isams_i->read_item = heap_cread_item;

    while (hci.more)
    {
        char this_name[INP_NAME_MAX];
        ISAMS_P isams_p;
        char *dict_info;

        strcpy (this_name, hci.cur_name);
	assert (hci.cur_name[1]);
        hi->no_diffs++;
        if (!(dict_info = dict_lookup (hi->reg->dict, hci.cur_name)))
        {
            isams_p = isams_merge (hi->reg->isams, isams_i);
            hi->no_insertions++;
            dict_insert (hi->reg->dict, this_name, sizeof(ISAMS_P), &isams_p);
        }
	else
	{
	    logf (LOG_FATAL, "isams doesn't support this kind of update");
	    break;
	}
    }
    xfree (isams_i);
    return 0;
} 

struct progressInfo {
    time_t   startTime;
    time_t   lastTime;
    off_t    totalBytes;
    off_t    totalOffset;
};

void progressFunc (struct key_file *keyp, void *info)
{
    struct progressInfo *p = (struct progressInfo *) info;
    time_t now, remaining;

    if (keyp->buf_size <= 0 || p->totalBytes <= 0)
        return ;
    time (&now);

    if (now >= p->lastTime+10)
    {
        p->lastTime = now;
        remaining = (time_t) ((now - p->startTime)*
            ((double) p->totalBytes/p->totalOffset - 1.0));
        if (remaining <= 130)
            logf (LOG_LOG, "Merge %2.1f%% completed; %ld seconds remaining",
                 (100.0*p->totalOffset) / p->totalBytes, (long) remaining);
        else
            logf (LOG_LOG, "Merge %2.1f%% completed; %ld minutes remaining",
	         (100.0*p->totalOffset) / p->totalBytes, (long) remaining/60);
    }
    p->totalOffset += keyp->buf_size;
}

#ifndef R_OK
#define R_OK 4
#endif

void zebra_index_merge (ZebraHandle zh)
{
    struct key_file **kf = 0;
    char rbuf[1024];
    int i, r;
    struct heap_info *hi;
    struct progressInfo progressInfo;
    int nkeys = zh->reg->key_file_no;
    int usefile; 
    
    logf (LOG_DEBUG, " index_merge called with nk=%d b=%p", 
                    nkeys, zh->reg->key_buf);
    if ( (nkeys==0) && (zh->reg->key_buf==0) )
        return; /* nothing to merge - probably flush after end-trans */
    
    usefile = (nkeys!=0); 

    if (usefile)
    {
        if (nkeys < 0)
        {
            char fname[1024];
            nkeys = 0;
            while (1)
            {
                extract_get_fname_tmp  (zh, fname, nkeys+1);
                if (access (fname, R_OK) == -1)
                        break;
                nkeys++;
            }
            if (!nkeys)
                return ;
        }
        kf = (struct key_file **) xmalloc ((1+nkeys) * sizeof(*kf));
        progressInfo.totalBytes = 0;
        progressInfo.totalOffset = 0;
        time (&progressInfo.startTime);
        time (&progressInfo.lastTime);
        for (i = 1; i<=nkeys; i++)
        {
            kf[i] = key_file_init (i, 8192, zh->res);
            kf[i]->readHandler = progressFunc;
            kf[i]->readInfo = &progressInfo;
            progressInfo.totalBytes += kf[i]->length;
            progressInfo.totalOffset += kf[i]->buf_size;
        }
        hi = key_heap_init (nkeys, key_qsort_compare);
        hi->reg = zh->reg;
        
        for (i = 1; i<=nkeys; i++)
            if ((r = key_file_read (kf[i], rbuf)))
                key_heap_insert (hi, rbuf, r, kf[i]);
    }  /* use file */
    else 
    { /* do not use file, read straight from buffer */
        hi = key_heap_init_buff (zh,key_qsort_compare);
        hi->reg = zh->reg;
    }
    if (zh->reg->isams)
	heap_inps (hi);
    if (zh->reg->isamc)
        heap_inpc (hi);
    if (zh->reg->isamb)
	heap_inpb (hi);
	
    if (usefile)
    {
        for (i = 1; i<=nkeys; i++)
        {
            extract_get_fname_tmp  (zh, rbuf, i);
            unlink (rbuf);
        }
        for (i = 1; i<=nkeys; i++)
            key_file_destroy (kf[i]);
        xfree (kf);
    }
    if (hi->no_iterations)
    { /* do not log if nothing happened */
        logf (LOG_LOG, "Iterations . . .%7d", hi->no_iterations);
        logf (LOG_LOG, "Distinct words .%7d", hi->no_diffs);
        logf (LOG_LOG, "Updates. . . . .%7d", hi->no_updates);
        logf (LOG_LOG, "Deletions. . . .%7d", hi->no_deletions);
        logf (LOG_LOG, "Insertions . . .%7d", hi->no_insertions);
    }
    zh->reg->key_file_no = 0;

    key_heap_destroy (hi, nkeys);
}

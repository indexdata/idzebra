/*
 * Copyright (C) 1994-1999, Index Data
 * All rights reserved.
 * Sebastian Hammer, Adam Dickmeiss, Heikki Levanto
 *
 * (log at the end)
 *
 * Bugs
 *  - Allocates a lot of memory for the merge process, but never releases it.
 *    Doesn't matter, as the program terminates soon after.  
 
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
#include "zserver.h"

#define KEY_SIZE (1+sizeof(struct it_key))
#define INP_NAME_MAX 768
#define INP_BUF_START 60000
#define INP_BUF_ADD  400000

static int no_diffs   = 0;
static int no_updates = 0;
static int no_deletions = 0;
static int no_insertions = 0;
static int no_iterations = 0;

struct key_file {
    int   no;            /* file no */
    off_t offset;        /* file offset */
    unsigned char *buf;  /* buffer block */
    size_t buf_size;     /* number of read bytes in block */
    size_t chunk;        /* number of bytes allocated */
    size_t buf_ptr;      /* current position in buffer */
    char *prev_name;     /* last word read */
    int   sysno;         /* last sysno */
    int   seqno;         /* last seqno */
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
    
    pre = res_get_def (zh->service->res, "keyTmpDir", ".");
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

struct key_file *key_file_init (int no, int chunk, Res res)
{
    struct key_file *f;

    f = (struct key_file *) xmalloc (sizeof(*f));
    f->res = res;
    f->sysno = 0;
    f->seqno = 0;
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
    case 192:
        d = ((c&63) << 8) + (key_file_getc (f) & 0xff);
        d = (d << 8) + (key_file_getc (f) & 0xff);
        d = (d << 8) + (key_file_getc (f) & 0xff);
        break;
    }
    return d;
}

int key_file_read (struct key_file *f, char *key)
{
    int i, d, c;
    struct it_key itkey;

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
        f->sysno = 0;
    }
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
}

struct heap_info {
    struct {
        struct key_file **file;
        char   **buf;
    } info;
    int    heapnum;
    int    *ptr;
    int    (*cmp)(const void *p1, const void *p2);
    Dict dict;
    ISAMS isams;
#if ZMBOL
    ISAM isam;
    ISAMC isamc;
    ISAMD isamd;
#endif
};

struct heap_info *key_heap_init (int nkeys,
                                 int (*cmp)(const void *p1, const void *p2))
{
    struct heap_info *hi;
    int i;

    hi = (struct heap_info *) xmalloc (sizeof(*hi));
    hi->info.file = (struct key_file **)
	xmalloc (sizeof(*hi->info.file) * (1+nkeys));
    hi->info.buf = (char **) xmalloc (sizeof(*hi->info.buf) * (1+nkeys));
    hi->heapnum = 0;
    hi->ptr = (int *) xmalloc (sizeof(*hi->ptr) * (1+nkeys));
    hi->cmp = cmp;
    for (i = 0; i<= nkeys; i++)
    {
        hi->ptr[i] = i;
        hi->info.buf[i] = (char *) xmalloc (INP_NAME_MAX);
    }
    return hi;
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

static int heap_read_one (struct heap_info *hi, char *name, char *key)
{
    int n, r;
    char rbuf[INP_NAME_MAX];
    struct key_file *kf;

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
    no_iterations++;
    return 1;
}

struct heap_cread_info {
    char prev_name[INP_NAME_MAX];
    char cur_name[INP_NAME_MAX];
    char *key;
    struct heap_info *hi;
    int mode;
    int more;
};
      
int heap_cread_item (void *vp, char **dst, int *insertMode)
{
    struct heap_cread_info *p = (struct heap_cread_info *) vp;
    struct heap_info *hi = p->hi;

    if (p->mode == 1)
    {
        *insertMode = p->key[0];
        memcpy (*dst, p->key+1, sizeof(struct it_key));
        (*dst) += sizeof(struct it_key);
        p->mode = 2;
        return 1;
    }
    strcpy (p->prev_name, p->cur_name);
    if (!(p->more = heap_read_one (hi, p->cur_name, p->key)))
        return 0;
    if (*p->cur_name && strcmp (p->cur_name, p->prev_name))
    {
        p->mode = 1;
        return 0;
    }
    *insertMode = p->key[0];
    memcpy (*dst, p->key+1, sizeof(struct it_key));
    (*dst) += sizeof(struct it_key);
    return 1;
}

#if ZMBOL
int heap_inpc (struct heap_info *hi)
{
    struct heap_cread_info hci;
    ISAMC_I isamc_i = (ISAMC_I) xmalloc (sizeof(*isamc_i));

    hci.key = (char *) xmalloc (KEY_SIZE);
    hci.mode = 1;
    hci.hi = hi;
    hci.more = heap_read_one (hi, hci.cur_name, hci.key);

    isamc_i->clientData = &hci;
    isamc_i->read_item = heap_cread_item;

    while (hci.more)
    {
        char this_name[INP_NAME_MAX];
        ISAMC_P isamc_p, isamc_p2;
        char *dict_info;

        strcpy (this_name, hci.cur_name);
	assert (hci.cur_name[1]);
        no_diffs++;
        if ((dict_info = dict_lookup (hi->dict, hci.cur_name)))
        {
            memcpy (&isamc_p, dict_info+1, sizeof(ISAMC_P));
            isamc_p2 = isc_merge (hi->isamc, isamc_p, isamc_i);
            if (!isamc_p2)
            {
                no_deletions++;
                if (!dict_delete (hi->dict, this_name))
                    abort();
            }
            else 
            {
                no_updates++;
                if (isamc_p2 != isamc_p)
                    dict_insert (hi->dict, this_name,
                                 sizeof(ISAMC_P), &isamc_p2);
            }
        } 
        else
        {
            isamc_p = isc_merge (hi->isamc, 0, isamc_i);
            no_insertions++;
            dict_insert (hi->dict, this_name, sizeof(ISAMC_P), &isamc_p);
        }
    }
    xfree (isamc_i);
    return 0;
} 

int heap_inpd (struct heap_info *hi)
{
    struct heap_cread_info hci;
    ISAMD_I isamd_i = (ISAMD_I) xmalloc (sizeof(*isamd_i));

    hci.key = (char *) xmalloc (KEY_SIZE);
    hci.mode = 1;
    hci.hi = hi;
    hci.more = heap_read_one (hi, hci.cur_name, hci.key);

    isamd_i->clientData = &hci;
    isamd_i->read_item = heap_cread_item;

    while (hci.more)
    {
        char this_name[INP_NAME_MAX];
        ISAMD_P isamd_p, isamd_p2;
        char *dict_info;

        strcpy (this_name, hci.cur_name);
	assert (hci.cur_name[1]);
        no_diffs++;
        if ((dict_info = dict_lookup (hi->dict, hci.cur_name)))
        {
            memcpy (&isamd_p, dict_info+1, sizeof(ISAMD_P));
            isamd_p2 = isamd_append (hi->isamd, isamd_p, isamd_i);
            if (!isamd_p2)
            {
                no_deletions++;
                if (!dict_delete (hi->dict, this_name))
                    abort();
            }
            else 
            {
                no_updates++;
                if (isamd_p2 != isamd_p)
                    dict_insert (hi->dict, this_name,
                                 sizeof(ISAMD_P), &isamd_p2);
            }
        } 
        else
        {
            isamd_p = isamd_append (hi->isamd, 0, isamd_i);
            no_insertions++;
            dict_insert (hi->dict, this_name, sizeof(ISAMD_P), &isamd_p);
        }
    }
    xfree (isamd_i);
    return 0;
} 

int heap_inp (struct heap_info *hi)
{
    char *info;
    char next_name[INP_NAME_MAX];
    char cur_name[INP_NAME_MAX];
    int key_buf_size = INP_BUF_START;
    int key_buf_ptr;
    char *next_key;
    char *key_buf;
    int more;
    
    next_key = (char *) xmalloc (KEY_SIZE);
    key_buf = (char *) xmalloc (key_buf_size);
    more = heap_read_one (hi, cur_name, key_buf);
    while (more)                   /* EOF ? */
    {
        int nmemb;
        key_buf_ptr = KEY_SIZE;
        while (1)
        {
            if (!(more = heap_read_one (hi, next_name, next_key)))
                break;
            if (*next_name && strcmp (next_name, cur_name))
                break;
            memcpy (key_buf + key_buf_ptr, next_key, KEY_SIZE);
            key_buf_ptr += KEY_SIZE;
            if (key_buf_ptr+(int) KEY_SIZE >= key_buf_size)
            {
                char *new_key_buf;
                new_key_buf = (char *) xmalloc (key_buf_size + INP_BUF_ADD);
                memcpy (new_key_buf, key_buf, key_buf_size);
                key_buf_size += INP_BUF_ADD;
                xfree (key_buf);
                key_buf = new_key_buf;
            }
        }
        no_diffs++;
        nmemb = key_buf_ptr / KEY_SIZE;
        assert (nmemb * (int) KEY_SIZE == key_buf_ptr);
        if ((info = dict_lookup (hi->dict, cur_name)))
        {
            ISAM_P isam_p, isam_p2;
            memcpy (&isam_p, info+1, sizeof(ISAM_P));
            isam_p2 = is_merge (hi->isam, isam_p, nmemb, key_buf);
            if (!isam_p2)
            {
                no_deletions++;
                if (!dict_delete (hi->dict, cur_name))
                    abort ();
            }
            else 
            {
                no_updates++;
                if (isam_p2 != isam_p)
                    dict_insert (hi->dict, cur_name, sizeof(ISAM_P), &isam_p2);
            }
        }
        else
        {
            ISAM_P isam_p;
            no_insertions++;
            isam_p = is_merge (hi->isam, 0, nmemb, key_buf);
            dict_insert (hi->dict, cur_name, sizeof(ISAM_P), &isam_p);
        }
        memcpy (key_buf, next_key, KEY_SIZE);
        strcpy (cur_name, next_name);
    }
    return 0;
}

#endif

int heap_inps (struct heap_info *hi)
{
    struct heap_cread_info hci;
    ISAMS_I isams_i = (ISAMS_I) xmalloc (sizeof(*isams_i));

    hci.key = (char *) xmalloc (KEY_SIZE);
    hci.mode = 1;
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
        no_diffs++;
        if (!(dict_info = dict_lookup (hi->dict, hci.cur_name)))
        {
            isams_p = isams_merge (hi->isams, isams_i);
            no_insertions++;
            dict_insert (hi->dict, this_name, sizeof(ISAMS_P), &isams_p);
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
    struct key_file **kf;
    char rbuf[1024];
    int i, r;
    struct heap_info *hi;
    struct progressInfo progressInfo;
    int nkeys = zh->key_file_no;
    
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
        kf[i] = key_file_init (i, 32768, zh->service->res);
        kf[i]->readHandler = progressFunc;
        kf[i]->readInfo = &progressInfo;
        progressInfo.totalBytes += kf[i]->length;
        progressInfo.totalOffset += kf[i]->buf_size;
    }
    hi = key_heap_init (nkeys, key_qsort_compare);
    hi->dict = zh->service->dict;
    hi->isams = zh->service->isams;
#if ZMBOL
    hi->isam = zh->service->isam;
    hi->isamc = zh->service->isamc;
    hi->isamd = zh->service->isamd;
#endif
    
    for (i = 1; i<=nkeys; i++)
        if ((r = key_file_read (kf[i], rbuf)))
            key_heap_insert (hi, rbuf, r, kf[i]);
    if (zh->service->isams)
	heap_inps (hi);
#if ZMBOL
    else if (zh->service->isamc)
        heap_inpc (hi);
    else if (zh->service->isam)
	heap_inp (hi);
    else if (zh->service->isamd)
	heap_inpd (hi);
#endif
	
    for (i = 1; i<=nkeys; i++)
    {
	extract_get_fname_tmp  (zh, rbuf, i);
        unlink (rbuf);
    }
    logf (LOG_LOG, "Iterations . . .%7d", no_iterations);
    logf (LOG_LOG, "Distinct words .%7d", no_diffs);
    logf (LOG_LOG, "Updates. . . . .%7d", no_updates);
    logf (LOG_LOG, "Deletions. . . .%7d", no_deletions);
    logf (LOG_LOG, "Insertions . . .%7d", no_insertions);
    zh->key_file_no = 0;
}

void key_input (BFiles bfs, int nkeys, int cache, Res res)
                
{
    Dict dict;
    ISAMS isams = NULL;
#if ZMBOL
    ISAM isam = NULL;
    ISAMC isamc = NULL;
    ISAMD isamd = NULL;
#endif
    struct key_file **kf;
    char rbuf[1024];
    int i, r;
    struct heap_info *hi;
    struct progressInfo progressInfo;

    if (nkeys < 0)
    {
        char fname[1024];
        nkeys = 0;
        while (1)
        {
            getFnameTmp (res, fname, nkeys+1);
            if (access (fname, R_OK) == -1)
                break;
            nkeys++;
        }
        if (!nkeys)
            return ;
    }
    dict = dict_open (bfs, FNAME_DICT, cache, 1, 0);
    if (!dict)
    {
        logf (LOG_FATAL, "dict_open fail");
        exit (1);
    }
    if (res_get_match (res, "isam", "s", ISAM_DEFAULT))
    {
	struct ISAMS_M_s isams_m;
        isams = isams_open (bfs, FNAME_ISAMS, 1,
			    key_isams_m (res, &isams_m));
        if (!isams)
        {
            logf (LOG_FATAL, "isams_open fail");
            exit (1);
        }
	logf (LOG_LOG, "isams opened");
    }
#if ZMBOL
    else if (res_get_match (res, "isam", "i", ISAM_DEFAULT))
    {
        isam = is_open (bfs, FNAME_ISAM, key_compare, 1,
			sizeof(struct it_key), res);
        if (!isam)
        {
            logf (LOG_FATAL, "is_open fail");
            exit (1);
        }
    }
    else if (res_get_match (res, "isam", "d", ISAM_DEFAULT))
    {
	struct ISAMD_M_s isamd_m;
        isamd = isamd_open (bfs, FNAME_ISAMD, 1,
			  key_isamd_m (res,&isamd_m));
        if (!isamd)
        {
            logf (LOG_FATAL, "isamd_open fail");
            exit (1);
        }
    }
    else if (res_get_match (res, "isam", "c", ISAM_DEFAULT))
    {
	struct ISAMC_M_s isamc_m;
        isamc = isc_open (bfs, FNAME_ISAMC, 1,
			  key_isamc_m (res, &isamc_m));
        if (!isamc)
        {
            logf (LOG_FATAL, "isc_open fail");
            exit (1);
        }
    }
#endif
    kf = (struct key_file **) xmalloc ((1+nkeys) * sizeof(*kf));
    progressInfo.totalBytes = 0;
    progressInfo.totalOffset = 0;
    time (&progressInfo.startTime);
    time (&progressInfo.lastTime);
    for (i = 1; i<=nkeys; i++)
    {
        kf[i] = key_file_init (i, 32768, res);
        kf[i]->readHandler = progressFunc;
        kf[i]->readInfo = &progressInfo;
        progressInfo.totalBytes += kf[i]->length;
        progressInfo.totalOffset += kf[i]->buf_size;
    }
    hi = key_heap_init (nkeys, key_qsort_compare);
    hi->dict = dict;
    hi->isams = isams;
#if ZMBOL
    hi->isam = isam;
    hi->isamc = isamc;
    hi->isamd = isamd;
#endif
    
    for (i = 1; i<=nkeys; i++)
        if ((r = key_file_read (kf[i], rbuf)))
            key_heap_insert (hi, rbuf, r, kf[i]);
    if (isams)
	heap_inps (hi);
#if ZMBOL
    else if (isamc)
        heap_inpc (hi);
    else if (isam)
	heap_inp (hi);
    else if (isamd)
	heap_inpd (hi);
#endif
	
    dict_close (dict);
    if (isams)
	isams_close (isams);
#if ZMBOL
    if (isam)
        is_close (isam);
    if (isamc)
        isc_close (isamc);
    if (isamd)
        isamd_close (isamd);
#endif
   
    for (i = 1; i<=nkeys; i++)
    {
        getFnameTmp (res, rbuf, i);
        unlink (rbuf);
    }
    logf (LOG_LOG, "Iterations . . .%7d", no_iterations);
    logf (LOG_LOG, "Distinct words .%7d", no_diffs);
    logf (LOG_LOG, "Updates. . . . .%7d", no_updates);
    logf (LOG_LOG, "Deletions. . . .%7d", no_deletions);
    logf (LOG_LOG, "Insertions . . .%7d", no_insertions);

    /* xmalloc_trav("unfreed"); while hunting leaks */     
}



/*
 * $Log: kinput.c,v $
 * Revision 1.43  2000-03-20 19:08:36  adam
 * Added remote record import using Z39.50 extended services and Segment
 * Requests.
 *
 * Revision 1.42  1999/12/01 21:58:48  adam
 * Proper handle of illegal use of isams.
 *
 * Revision 1.41  1999/11/30 13:48:03  adam
 * Improved installation. Updated for inclusion of YAZ header files.
 *
 * Revision 1.40  1999/09/08 12:12:39  adam
 * Removed log message.
 *
 * Revision 1.39  1999/08/18 10:39:20  heikki
 * Added a comment on memory leaks
 *
 * Revision 1.38  1999/08/18 08:38:04  heikki
 * Memory leak hunting
 *
 * Revision 1.37  1999/07/14 13:21:34  heikki
 * Added isam-d files. Compiles (almost) clean. Doesn't work at all
 *
 * Revision 1.36  1999/07/14 10:59:26  adam
 * Changed functions isc_getmethod, isams_getmethod.
 * Improved fatal error handling (such as missing EXPLAIN schema).
 *
 * Revision 1.35  1999/06/30 15:07:23  heikki
 * Adding isamh stuff
 *
 * Revision 1.34  1999/05/26 07:49:13  adam
 * C++ compilation.
 *
 * Revision 1.33  1999/05/15 14:36:38  adam
 * Updated dictionary. Implemented "compression" of dictionary.
 *
 * Revision 1.32  1999/05/12 13:08:06  adam
 * First version of ISAMS.
 *
 * Revision 1.31  1999/02/02 14:50:56  adam
 * Updated WIN32 code specific sections. Changed header.
 *
 * Revision 1.30  1998/10/28 10:53:57  adam
 * Added type cast to prevent warning.
 *
 * Revision 1.29  1998/06/11 15:41:39  adam
 * Minor changes.
 *
 * Revision 1.28  1998/03/05 08:45:12  adam
 * New result set model and modular ranking system. Moved towards
 * descent server API. System information stored as "SGML" records.
 *
 * Revision 1.27  1998/02/17 10:32:52  adam
 * Fixed bug: binary files weren't opened with flag b on NT.
 *
 * Revision 1.26  1998/01/29 13:39:13  adam
 * Compress ISAM is default.
 *
 * Revision 1.25  1997/09/17 12:19:14  adam
 * Zebra version corresponds to YAZ version 1.4.
 * Changed Zebra server so that it doesn't depend on global common_resource.
 *
 * Revision 1.24  1997/09/09 13:38:07  adam
 * Partial port to WIN95/NT.
 *
 * Revision 1.23  1997/09/04 13:57:39  adam
 * Added O_BINARY for open calls.
 *
 * Revision 1.22  1997/02/12 20:39:45  adam
 * Implemented options -f <n> that limits the log to the first <n>
 * records.
 * Changed some log messages also.
 *
 * Revision 1.21  1996/11/08 11:10:23  adam
 * Buffers used during file match got bigger.
 * Compressed ISAM support everywhere.
 * Bug fixes regarding masking characters in queries.
 * Redesigned Regexp-2 queries.
 *
 * Revision 1.20  1996/11/01 08:58:41  adam
 * Interface to isamc system now includes update and delete.
 *
 * Revision 1.19  1996/10/29 14:09:46  adam
 * Use of cisam system - enabled if setting isamc is 1.
 *
 * Revision 1.18  1996/06/04 10:18:59  adam
 * Minor changes - removed include of ctype.h.
 *
 * Revision 1.17  1996/05/14  15:47:07  adam
 * Cleanup of various buffer size entities.
 *
 * Revision 1.16  1996/04/09  10:05:20  adam
 * Bug fix: prev_name buffer possibly too small; allocated in key_file_init.
 *
 * Revision 1.15  1996/03/21  14:50:09  adam
 * File update uses modify-time instead of change-time.
 *
 * Revision 1.14  1996/02/07  14:06:37  adam
 * Better progress report during register merge.
 * New command: clean - removes temporary shadow files.
 *
 * Revision 1.13  1996/02/05  12:30:00  adam
 * Logging reduced a bit.
 * The remaining running time is estimated during register merge.
 *
 * Revision 1.12  1995/12/06  17:49:19  adam
 * Uses dict_delete now.
 *
 * Revision 1.11  1995/12/06  16:06:43  adam
 * Better diagnostics. Work on 'real' dictionary deletion.
 *
 * Revision 1.10  1995/12/06  12:41:22  adam
 * New command 'stat' for the index program.
 * Filenames can be read from stdin by specifying '-'.
 * Bug fix/enhancement of the transformation from terms to regular
 * expressons in the search engine.
 *
 * Revision 1.9  1995/10/10  12:24:39  adam
 * Temporary sort files are compressed.
 *
 * Revision 1.8  1995/10/04  16:57:19  adam
 * Key input and merge sort in one pass.
 *
 * Revision 1.7  1995/10/02  15:18:52  adam
 * New member in recRetrieveCtrl: diagnostic.
 *
 * Revision 1.6  1995/09/29  15:51:56  adam
 * First work on multi-way read.
 *
 * Revision 1.5  1995/09/29  14:01:43  adam
 * Bug fixes.
 *
 * Revision 1.4  1995/09/28  14:22:57  adam
 * Sort uses smaller temporary files.
 *
 * Revision 1.3  1995/09/06  16:11:17  adam
 * Option: only one word key per file.
 *
 * Revision 1.2  1995/09/04  12:33:42  adam
 * Various cleanup. YAZ util used instead.
 *
 * Revision 1.1  1995/09/04  09:10:37  adam
 * More work on index add/del/update.
 * Merge sort implemented.
 * Initial work on z39 server.
 *
 */


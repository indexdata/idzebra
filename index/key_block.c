/* $Id: key_block.c,v 1.12 2008-01-09 23:00:13 adam Exp $
   Copyright (C) 1995-2007
   Index Data ApS

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

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <assert.h>
#include <ctype.h>

#if YAZ_POSIX_THREADS
#include <pthread.h>
#endif

#include "key_block.h"
#include <yaz/nmem.h>
#include <yaz/xmalloc.h>

struct zebra_key_block {
    char **key_buf;
    size_t ptr_top;
    size_t ptr_i;
    size_t key_buf_used;
    int key_file_no;
    char *key_tmp_dir;
    int use_threads;
    char **alt_buf;
#if YAZ_POSIX_THREADS
    char **thread_key_buf;
    size_t thread_ptr_top;
    size_t thread_ptr_i;
    int exit_flag;
    pthread_t thread_id;
    pthread_mutex_t mutex;

    pthread_cond_t work_available;

    pthread_cond_t cond_sorting;
    int is_sorting;
#endif
};

#define ENCODE_BUFLEN 768
struct encode_info {
    void *encode_handle;
    void *decode_handle;
    char buf[ENCODE_BUFLEN];
};

#define USE_SHELLSORT 0

#if USE_SHELLSORT
static void shellsort(void *ar, int r, size_t s,
                      int (*cmp)(const void *a, const void *b))
{
    char *a = ar;
    char v[100];
    int h, i, j, k;
    static const int incs[16] = { 1391376, 463792, 198768, 86961, 33936,
                                  13776, 4592, 1968, 861, 336, 
                                  112, 48, 21, 7, 3, 1 };
    for ( k = 0; k < 16; k++)
        for (h = incs[k], i = h; i < r; i++)
        { 
            memcpy (v, a+s*i, s);
            j = i;
            while (j > h && (*cmp)(a + s*(j-h), v) > 0)
            {
                memcpy (a + s*j, a + s*(j-h), s);
                j -= h;
            }
            memcpy (a+s*j, v, s);
        } 
}
#endif


static void encode_key_init(struct encode_info *i)
{
    i->encode_handle = iscz1_start();
    i->decode_handle = iscz1_start();
}

static void encode_key_write(const char *k, struct encode_info *i, FILE *outf)
{
    struct it_key key;
    char *bp = i->buf, *bp0;
    const char *src = (char *) &key;
    size_t klen = strlen(k);
    
    if (fwrite (k, klen+1, 1, outf) != 1)
    {
        yaz_log (YLOG_FATAL|YLOG_ERRNO, "fwrite");
        zebra_exit("encode_key_write");
    }

    k = k + klen+1;

    /* and copy & align key so we can mangle */
    memcpy (&key, k+1, sizeof(struct it_key));  /* *k is insert/delete */

#if 0
    /* debugging */
    key_logdump_txt(YLOG_LOG, &key, *k ? "i" : "d");
#endif
    assert(key.mem[0] >= 0);

    bp0 = bp++;
    iscz1_encode(i->encode_handle, &bp, &src);

    *bp0 = (*k * 128) + bp - bp0 - 1; /* length and insert/delete combined */
    if (fwrite (i->buf, bp - i->buf, 1, outf) != 1)
    {
        yaz_log (YLOG_FATAL|YLOG_ERRNO, "fwrite");
        zebra_exit("encode_key_write");
    }

#if 0
    /* debugging */
    if (1)
    {
	struct it_key key2;
	const char *src = bp0+1;
	char *dst = (char*) &key2;
	iscz1_decode(i->decode_handle, &dst, &src);

	key_logdump_txt(YLOG_LOG, &key2, *k ? "i" : "d");

	assert(key2.mem[1]);
    }
#endif
}

static void encode_key_flush (struct encode_info *i, FILE *outf)
{ 
    iscz1_stop(i->encode_handle);
    iscz1_stop(i->decode_handle);
}

void key_block_flush_int(zebra_key_block_t p,
                         char **key_buf, size_t ptr_top, size_t ptr_i);

#if YAZ_POSIX_THREADS
static void *thread_func(void *vp)
{
    zebra_key_block_t p = (zebra_key_block_t) vp;
    while (1)
    {
        pthread_mutex_lock(&p->mutex);
        
        while (!p->is_sorting && !p->exit_flag)
            pthread_cond_wait(&p->work_available, &p->mutex);

        if (p->exit_flag)
            break;
            
        pthread_mutex_unlock(&p->mutex);
        
        key_block_flush_int(p, p->thread_key_buf, 
                            p->thread_ptr_top, p->thread_ptr_i);
        
        pthread_mutex_lock(&p->mutex);
        p->is_sorting = 0;
        pthread_cond_signal(&p->cond_sorting);
        pthread_mutex_unlock(&p->mutex);        
    }
    pthread_mutex_unlock(&p->mutex);
    return 0;
}
#endif

zebra_key_block_t key_block_create(int mem, const char *key_tmp_dir,
                                   int use_threads)
{
    zebra_key_block_t p = xmalloc(sizeof(*p));

#if YAZ_POSIX_THREADS
    /* we'll be making two memory areas so cut in half */
    if (use_threads)
        mem = mem / 2;
#endif
    p->key_buf = (char**) xmalloc (mem);
    p->ptr_top = mem/sizeof(char*);
    p->ptr_i = 0;
    p->key_buf_used = 0;
    p->key_tmp_dir = xstrdup(key_tmp_dir);
    p->key_file_no = 0;
    p->alt_buf = 0;
    p->use_threads = 0;
    if (use_threads)
    {
#if YAZ_POSIX_THREADS
        p->use_threads = use_threads;
        p->is_sorting = 0;
        p->exit_flag = 0;
        pthread_mutex_init(&p->mutex, 0);
        pthread_cond_init(&p->work_available, 0);
        pthread_cond_init(&p->cond_sorting, 0);
        pthread_create(&p->thread_id, 0, thread_func, p);
        p->alt_buf = (char**) xmalloc (mem);
#endif
    }
    yaz_log(YLOG_DEBUG, "key_block_create t=%d", p->use_threads);
    return p;
}

void key_block_destroy(zebra_key_block_t *pp)
{
    zebra_key_block_t p = *pp;
    if (p)
    {
        if (p->use_threads)
        {
#if YAZ_POSIX_THREADS
            pthread_mutex_lock(&p->mutex);
            
            while (p->is_sorting)
                pthread_cond_wait(&p->cond_sorting, &p->mutex);
            
            p->exit_flag = 1;
            
            pthread_cond_broadcast(&p->work_available);
            
            pthread_mutex_unlock(&p->mutex);
            pthread_join(p->thread_id, 0);
            pthread_cond_destroy(&p->work_available);
            pthread_cond_destroy(&p->cond_sorting);
            pthread_mutex_destroy(&p->mutex);
            
#endif
            xfree(p->alt_buf);
        }
        xfree(p->key_buf);
        xfree(p->key_tmp_dir);
        xfree(p);
        *pp = 0;
    }
}

void key_block_write(zebra_key_block_t p, zint sysno, struct it_key *key_in,
                     int cmd, const char *str_buf, size_t str_len,
                     zint staticrank, int static_rank_enable)
{
    int ch;
    int i, j = 0;
    struct it_key key_out;

    if (p->key_buf_used + 1024 > (p->ptr_top -p->ptr_i)*sizeof(char*))
        key_block_flush(p, 0);
    ++(p->ptr_i);
    assert(p->ptr_i > 0);
    (p->key_buf)[p->ptr_top - p->ptr_i] =
        (char*)p->key_buf + p->key_buf_used;
    
    /* key_in->mem[0] ord/ch */
    /* key_in->mem[1] filter specified record ID */
    
    /* encode the ordinal value (field/use/attribute) .. */
    ch = CAST_ZINT_TO_INT(key_in->mem[0]);
    p->key_buf_used +=
        key_SU_encode(ch, (char*)p->key_buf +
                      p->key_buf_used);
    
    /* copy the 0-terminated stuff from str to output */
    memcpy((char*)p->key_buf + p->key_buf_used, str_buf, str_len);
    p->key_buf_used += str_len;
    ((char*)p->key_buf)[(p->key_buf_used)++] = '\0';
    
    /* the delete/insert indicator */
    ((char*)p->key_buf)[(p->key_buf_used)++] = cmd;
    
    if (static_rank_enable)
    {
        assert(staticrank >= 0);
        key_out.mem[j++] = staticrank;
    }
    
    if (key_in->mem[1]) /* filter specified record ID */
        key_out.mem[j++] = key_in->mem[1];
    else
        key_out.mem[j++] = sysno;
    for (i = 2; i < key_in->len; i++)
        key_out.mem[j++] = key_in->mem[i];
    key_out.len = j;
    
    memcpy((char*)p->key_buf + p->key_buf_used,
           &key_out, sizeof(key_out));
    (p->key_buf_used) += sizeof(key_out);
}


void key_block_flush_int(zebra_key_block_t p,
                         char **key_buf, size_t ptr_top, size_t ptr_i)
{
    FILE *outf;
    char out_fname[200];
    char *prevcp, *cp;
    struct encode_info encode_info;

    if (ptr_i == 0)
        return ;
        
    (p->key_file_no)++;
    yaz_log(YLOG_DEBUG, "sorting section %d", (p->key_file_no));

    assert(ptr_i > 0);

#if USE_SHELLSORT
    shellsort(key_buf + ptr_top - ptr_i, ptr_i,
              sizeof(char*), key_qsort_compare);
#else
    qsort(key_buf + ptr_top - ptr_i, ptr_i,
          sizeof(char*), key_qsort_compare);
#endif
    sprintf(out_fname, "%s/key%d.tmp", p->key_tmp_dir, p->key_file_no);

    if (!(outf = fopen (out_fname, "wb")))
    {
        yaz_log (YLOG_FATAL|YLOG_ERRNO, "fopen %s", out_fname);
        zebra_exit("key_block_flush");
    }
    yaz_log(YLOG_DEBUG, "writing section %d", p->key_file_no);
    prevcp = cp = (key_buf)[ptr_top - ptr_i];
    
    encode_key_init (&encode_info);
    encode_key_write (cp, &encode_info, outf);
    
    while (--ptr_i > 0)
    {
        cp = (key_buf)[ptr_top - ptr_i];
        if (strcmp (cp, prevcp))
        {
            encode_key_flush ( &encode_info, outf);
            encode_key_init (&encode_info);
            encode_key_write (cp, &encode_info, outf);
            prevcp = cp;
        }
        else
            encode_key_write (cp + strlen(cp), &encode_info, outf);
    }
    encode_key_flush ( &encode_info, outf);
    if (fclose (outf))
    {
        yaz_log (YLOG_FATAL|YLOG_ERRNO, "fclose %s", out_fname);
        zebra_exit("key_block_flush");
    }
    yaz_log(YLOG_DEBUG, "finished section %d", p->key_file_no);
}

void key_block_flush(zebra_key_block_t p, int is_final)
{
    if (!p)
        return;

    if (p->use_threads)
    {
#if YAZ_POSIX_THREADS
        char **tmp;
    
        pthread_mutex_lock(&p->mutex);
        
        while (p->is_sorting)
            pthread_cond_wait(&p->cond_sorting, &p->mutex);
        
        p->is_sorting = 1;
        
        p->thread_ptr_top = p->ptr_top;
        p->thread_ptr_i = p->ptr_i;
        p->thread_key_buf = p->key_buf;
        
        tmp = p->key_buf;
        p->key_buf = p->alt_buf;
        p->alt_buf = tmp;
        
        pthread_cond_signal(&p->work_available);
        
        if (is_final)
        {
            while (p->is_sorting)
                pthread_cond_wait(&p->cond_sorting, &p->mutex);
        }
        pthread_mutex_unlock(&p->mutex);
#endif
    }
    else
        key_block_flush_int(p, p->key_buf, p->ptr_top, p->ptr_i);
    p->ptr_i = 0;
    p->key_buf_used = 0;
}

int key_block_get_no_files(zebra_key_block_t p)
{
    if (p)
        return p->key_file_no;
    return 0;
}

/*
 * Local variables:
 * c-basic-offset: 4
 * indent-tabs-mode: nil
 * End:
 * vim: shiftwidth=4 tabstop=8 expandtab
 */



#include <yaz/xmalloc.h>
#include <isamb.h>

struct ISAMB_s {
    BFiles bfs;
    ISAMC_M method;
};

typedef unsigned char *Bpage;

ISAMB isamb_open (BFiles bfs, const char *name, ISAMC_M method)
{
    ISAMB isamb = xmalloc (sizeof(*isamb));

    isamb->bfs = bfs;
    isamb->method = (ISAMC_M) xmalloc (sizeof(*method));
    memcpy (isamb->method, method, sizeof(*method));
    return isamb;
}

void isamb_close (ISAMB isamb)
{
    xfree (isamb->method);
    xfree (isamb);
}

#if 0
/* read page at pos */
void isamb_get_block (ISAMB is, ISAMB_pos pos, Bpage *page)
{
}

/* alloc page */
ISAMB_pos isamb_alloc_block (ISAMB is, int block_size, Bpage *page)
{
}

#define isamb_page_set_leaf (p)   0[p] = 1
#define isamb_page_set_noleaf (p) 0[p] = 0
#define isamb_page_datalist (4+p)

static void isamb_page_set_no(Bpage page, int no)
{
    page[1] = no & 255;
    page[2] = (no >> 8) & 255;
    page[3] = (no >> 16) & 255;
}

static int isamb_page_get_no(Bpage page)
{
    return page[1] + 256*page[2] + 65536*page[3];
}

void isamb_insert_sub(ISAMB is, ISAMB_pos *pos, const void *data)
{
    const char *src;
    char dst[200];
    int no, i;
    
    isamb_get_block (is, *pos, &page);
    if (!isamb_page_isleaf (page))
    {
	ISAMB_pos subptr;
	src = isamb_page_datalist (page);
	no = isamb_page_get_no (page);
	decodeClientData = (*is->method->code_start)(ISAMC_DECODE);
	
	isamb_read_subptr (&subptr, &src);
	for (i = 0; i<no; i++)
	{
	    const char *src0 = src;
	    
	    (*is->method->code_item)(ISAMC_DECODE, decodeClientData,
				     dst, &src);
	    if ((*is->method->compare_item)(data, dst) < 0)
		break;
	    
	    isamb_read_subptr (&subptr, src);
	}
	isamb_insert_sub (is, subptr, data);
	*pos = subptr;
	(*is->method->code_stop)(ISAMC_DECODE, decodeClientData);
    }
    else
    {
	src = isamb_page_datalist (page);
	no = isamb_page_get_no (page);
	decodeClientData = (*is->method->code_start)(ISAMC_DECODE);
	diff = -1;
	for (i = 0; i<no; i++)
	{
	    int diff;
	    (*is->method->code_item)(ISAMC_DECODE, decodeClientData,
				     dst, &src);
	    diff = (*is->method->compare_item)(data, dst);
	    if (diff <= 0)
		break;
	}
	if (diff < 0)
	{
	    int j;
	    src = isamb_page_datalist (page);
	    page2 = isamb_page_dup (is, page);
	    dst2 = isamb_page_datalist (page2);
	    src2 = data;
	    for (j = 0; j <= no; j++)
	    {
		if ( i == j)
		    (*is->method->code_item)(ISAMC_ENCODE, encodeClientData,
					     &dst2, &src2);
		if (j < no)
		{
		    char *dst0 = dst;
		    (*is->method->code_item)(ISAMC_DECODE, decodeClientData,
					     &dst, &src);
		    (*is->method->code_item)(ISAMC_ENCODE, encodeClientData,
					     &dst2, &dst0);
		}
	    }
	}
    }
}
/* insert data(input) in table is(input) at pos(input/output) */
int isamb_insert (ISAMB is, ISAMB_pos *pos, const void *data)
{
    void *decodeClientData;

    Bpage page;
    if (*pos == 0)
    {
	*pos = isamb_alloc_block (is, 1024, &page);
	isamb_page_set_leaf (page);
	isamb_page_set_no (page, 0);
    }
    else     /* find leaf ... */
    {
	isamb_insert_sub (is, pos, const void *data);

    }
}
#endif

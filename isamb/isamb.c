#if 0
#include <isamb.h>

ISAMB isamb_open (const char *name, ISAMC_M method)
{
    ISAMB isamb = xmalloc (sizeof(*isamb));
    return isamb;
}

int isamb_insert (ISAMB is, ISAMB_pos *pos, const void *data)
{
    void *decodeClientData;

    void *page;
    if (*pos == 0)
    {
	*pos = isamb_alloc_block (is, isamb->block_size[0], &page);
	isamb_page_set_leaf (page);
	isamb_page_set_no (page, 0);
    }
    else     /* find leaf ... */
    {
	const char *src;
	char dst[200];
	int no, i;
	while (1)
	{
	    ISAMB_pos subptr;
	    
	    isamb_get_block (is, *pos, &page);
	    if (isamb_page_isleaf (page))
		break;
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
	    *pos = subptr;
	    (*is->method->code_stop)(ISAMC_DECODE, decodeClientData);
	}
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
#endif

#include <sys/types.h>
#include <fcntl.h>
#include <unistd.h>
#include <stdio.h>
#include <stdlib.h>

#include <dict.h>

int dict_bf_close (Dict_BFile dbf)
{
    int i;
    dict_bf_flush_blocks (dbf, -1);
    
    xfree (dbf->all_blocks);
    xfree (dbf->all_data);
    xfree (dbf->hash_array);
    i = bf_close (dbf->bf);
    xfree (dbf);
    return i;
}

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
    
    free (dbf->all_blocks);
    free (dbf->all_data);
    free (dbf->hash_array);
    i = bf_close (dbf->bf);
    free (dbf);
    return i;
}

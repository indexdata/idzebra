#include "../index/index.h"


void tst_encode(int num)
{
    struct it_key key;
    int i;
    void *codec_handle =iscz1_code_start(ISAMC_ENCODE);

    for (i = 0; i<num; i++)
    {
	char dst_buf[200];
	char *dst = dst_buf;
	char *src = (char*)  &key;

	key.sysno = num;
	key.seqno = 1;	
	iscz1_code_item (ISAMC_ENCODE, codec_handle, &dst, &src);
    }
    iscz1_code_stop(ISAMC_ENCODE, codec_handle);
}

int main(int argc, char **argv)
{
    tst_encode(1000000);
    exit(0);
}
    

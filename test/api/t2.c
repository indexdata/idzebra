/*
 * $Id: t2.c,v 1.2 2002-02-20 23:07:54 adam Exp $
 */

#include <zebraapi.h>

int main(int argc, char **argv)
{
    ZebraService zs;
    ZebraHandle zh;
    const char *myrec =
        "<gils>\n"
        "  <title>My title</title>\n"
        "</gils>\n";
    ODR odr_input, odr_output;

    nmem_init ();

    odr_input = odr_createmem (ODR_DECODE);    
    odr_output = odr_createmem (ODR_ENCODE);    
    
    zs = zebra_start("t2.cfg");
    zh = zebra_open (zs);
    
    zebra_begin_trans (zh);
    zebra_record_insert (zh, myrec, strlen(myrec));
    zebra_end_trans (zh);
    zebra_commit (zh);
    zebra_close (zh);
    zebra_stop (zs);

    odr_destroy (odr_input);
    odr_destroy (odr_output);

    nmem_exit ();
    xmalloc_trav ("x");
    exit (0);
}

/*
 * $Id: t1.c,v 1.1 2002-02-20 17:30:02 adam Exp $
 */

#include <zebraapi.h>

int main(int argc, char **argv)
{
    ZebraService zs;
    ZebraHandle zh;

    nmem_init();
    zs = zebra_start("t1.cfg");
    zh = zebra_open (zs);
    
    zebra_close (zh);
    zebra_stop (zs);
    nmem_exit ();
    xmalloc_trav ("x");
    exit (0);
}

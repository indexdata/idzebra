/*
 * $Id: testclient.c,v 1.2 2002-10-23 13:25:00 heikki Exp $
 *
 * Synchronous single-target client doing search (but no yet retrieval)
 */

#include <stdlib.h>
#include <stdio.h>
#include <yaz/xmalloc.h>
#include <yaz/zoom.h>

int main(int argc, char **argv)
{
    ZOOM_connection z;
    ZOOM_resultset r;
    int error;
    const char *errmsg, *addinfo;
    int sec;

    if ( (argc != 3) && (argc !=4) )
    {
        fprintf (stderr, "usage:\n%s target query [delay]\n", *argv);
        fprintf (stderr, " eg.  bagel.indexdata.dk/gils computer\n");
        exit (1);
    }
    z = ZOOM_connection_new (argv[1], 0);
    
    if ((error = ZOOM_connection_error(z, &errmsg, &addinfo)))
    {
	fprintf (stderr, "Error: %s (%d) %s\n", errmsg, error, addinfo);
	exit (2);
    }

    r = ZOOM_connection_search_pqf (z, argv[2]);
    if ((error = ZOOM_connection_error(z, &errmsg, &addinfo)))
	fprintf (stderr, "Error: %s (%d) %s\n", errmsg, error, addinfo);
    else
	printf ("Result count: %d\n", ZOOM_resultset_size(r));
    if (argc==4)
    {
        sec=atoi(argv[3]);
	if (sec <= 0)
	    sec=3;
	sleep(sec);
    }
    ZOOM_resultset_destroy (r);
    ZOOM_connection_destroy (z);
    exit (0);
}

/*
 * $Id: testclient.c,v 1.3 2002-10-23 13:41:15 adam Exp $
 *
 * Synchronous single-target client doing search (but no yet retrieval)
 */

#include <stdlib.h>
#include <stdio.h>
#include <yaz/xmalloc.h>
#include <yaz/options.h>
#include <yaz/zoom.h>

char *prog = "testclient";

int main(int argc, char **argv)
{
    ZOOM_connection z;
    ZOOM_resultset r;
    int error;
    const char *errmsg, *addinfo;
    char *query = 0;
    char *target = 0;
    char *arg;
    int delay_sec = 0;
    int ret;

    while ((ret = options("d:", argv, argc, &arg)) != -2)
    {
        switch (ret)
        {
        case 0:
            if (!target)
                target = xstrdup(arg);
            else if (!query)
                query = xstrdup(arg);
            break;
        case 'd':
            delay_sec = atoi(arg);
            break;
        default:
            printf ("%s: unknown option %s\n", prog, arg);
            printf ("usage:\n%s [options] target query \n", prog);
            printf (" eg.  bagel.indexdata.dk/gils computer\n");
            exit (1);
        }
    }

    if (!target || !target)
    {
        printf ("%s: missing target/query\n", prog);
        printf ("usage:\n%s [options] target query \n", prog);
        printf (" eg.  bagel.indexdata.dk/gils computer\n");
        exit (3);
    }
    z = ZOOM_connection_new (target, 0);
    
    if ((error = ZOOM_connection_error(z, &errmsg, &addinfo)))
    {
	printf ("Error: %s (%d) %s\n", errmsg, error, addinfo);
	exit (2);
    }

    r = ZOOM_connection_search_pqf (z, query);
    if ((error = ZOOM_connection_error(z, &errmsg, &addinfo)))
	fprintf (stderr, "Error: %s (%d) %s\n", errmsg, error, addinfo);
    else
	printf ("Result count: %d\n", ZOOM_resultset_size(r));
    
    if (delay_sec > 0)
	sleep(delay_sec);
    ZOOM_resultset_destroy (r);
    ZOOM_connection_destroy (z);
    exit (0);
}

/*
 * $Id: testclient.c,v 1.5 2002-11-09 22:26:19 adam Exp $
 *
 * Z39.50 client specifically for Zebra testing.
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
    int retrieve_number = 0;
    int retrieve_offset = 0;
    char *format = 0;
    int pos;
    int check_count = -1;
    int exit_code = 0;

    while ((ret = options("d:n:o:f:c:", argv, argc, &arg)) != -2)
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
        case 'n':
            retrieve_number = atoi(arg);
            break;
        case 'o':
            retrieve_offset = atoi(arg);
            break;
        case 'f':
            format = xstrdup(arg);
            break;
        case 'c':
	    check_count = atoi(arg);
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
        printf ("Options:\n");
        printf (" -n num       number of records to fetch. Default: 0.\n");
        printf (" -o off       offset for records - counting from 0.\n");
        printf (" -f format    set record syntax. Default: none\n");
        printf (" -d sec       delay a number of seconds before exit.\n");
        printf ("Options\n");
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
    {
	printf ("Error: %s (%d) %s\n", errmsg, error, addinfo);
	if (check_count != -1)
            exit_code = 10;
    }
    else
    {
	printf ("Result count: %d\n", ZOOM_resultset_size(r));
	if (check_count != -1 && check_count != ZOOM_resultset_size(r))
            exit_code = 10;
    }
    if (format)
        ZOOM_resultset_option_set(r, "preferredRecordSyntax", format);
    for (pos = 0; pos < retrieve_number; pos++)
    {
        size_t len;
        const char *rec =
            ZOOM_record_get(
                ZOOM_resultset_record(r, pos + retrieve_offset),
                "render", &len);
        
        if (rec)
            fwrite (rec, 1, len, stdout);
    }
    if (delay_sec > 0)
	sleep(delay_sec);
    ZOOM_resultset_destroy (r);
    ZOOM_connection_destroy (z);
    exit (0);
}

/*
 * Copyright (C) 1994, Index Data I/S 
 * All rights reserved.
 * Sebastian Hammer, Adam Dickmeiss
 *
 * $Log: isam.c,v $
 * Revision 1.1  1994-09-12 08:02:13  quinn
 * Not functional yet
 *
 */

#include <stdlib.h>
#include <string.h>
#include <ctype.h>

#include <util.h>
#include "isutil.h"
#include <bfile.h>
#include <isam.h>
#include <common.h>

static int splitargs(const char *s, char *bf[], int max)
{
    int ct = 0;
    for (;;)
    {
    	while (*s && isspace(*s))
	    s++;
    	bf[ct] = (char *) s;
	if (!*s)
		return ct;
	ct++;
	if (ct > max)
	{
	    log(LOG_WARN, "Ignoring extra args to is resource");
	    bf[ct] = '\0';
	    return(ct - 1);
	}
	while (*s && !isspace(*s))
	    s++;
    }
}

/*
 * Open isam file.
 * Process resources.
 */
ISAM is_open(const char *name, int writeflag)
{
    ISAM new;
    char *nm, *r, *pp[IS_MAX_BLOCKTYPES+1], m[2];
    int num, size, rs, tmp, i;

    log(LOG_DEBUG, "is_open(%s, %s)", name, writeflag ? "RW" : "RDONLY");
    new = xmalloc(sizeof(*new));
    new->writeflag = writeflag;

    /* determine number and size of blocktypes */
    if (!(r = res_get(common_resource, nm = strconcat(name, ".",
	"blocktypes", 0))) || !(num = splitargs(r, pp, IS_MAX_BLOCKTYPES)))
    {
    	log(LOG_FATAL, "Failed to locate resource %s", nm);
    	return 0;
    }
    new->num_types = num;
    for (i = 0; i < num; i++)
    {
    	if ((rs = sscanf(pp[i], "%d%1[bBkKmM]", &size, m)) < 1)
    	{
	    log(LOG_FATAL, "Error in resource %s: %s", r, pp[i]);
	    return 0;
	}
	if (rs == 1)
		*m = 'b';
	switch (*m)
	{
		case 'b': case 'B':
		    new->types[i].blocksize = size; break;
		case 'k': case 'K':
		    new->types[i].blocksize = size * 1024; break;
		case 'm': case 'M':
		    new->types[i].blocksize = size * 1048576; break;
		default:
		    log(LOG_FATAL, "Illegal size suffix: %c", *m);
		    return 0;
	}
	m[0] = 'A' + i;
	m[1] = '\0';
	if (!(new->types[i].bf = bf_open(strconcat(name, m, 0), 
	    new->types[i].blocksize, writeflag)))
	{
	    log(LOG_FATAL, "bf_open failed");
	    return 0;
	}
    }

    /* determine nice fill rates */
    if (!(r = res_get(common_resource, nm = strconcat(name, ".",
	"nicefill", 0))) || !(num = splitargs(r, pp, IS_MAX_BLOCKTYPES)))
    {
    	log(LOG_FATAL, "Failed to locate resource %s", nm);
    	return 0;
    }
    if (num < new->num_types)
    {
    	log(LOG_FATAL, "Not enough elements in %s", nm);
    	return 0;
    }
    for (i = 0; i < num; i++)
    {
    	if ((rs = sscanf(pp[i], "%d", &tmp)) < 1)
    	{
	    log(LOG_FATAL, "Error in resource %s: %s", r, pp[i]);
	    return 0;
	}
	new->types[i].nice_keys_block = tmp;
    }

    /* determine max keys/blocksize */
    if (!(r = res_get(common_resource, nm = strconcat(name, ".",
	"maxkeys", 0))) || !(num = splitargs(r, pp, IS_MAX_BLOCKTYPES)))
    {
    	log(LOG_FATAL, "Failed to locate resource %s", nm);
    	return 0;
    }
    if (num < new->num_types -1)
    {
    	log(LOG_FATAL, "Not enough elements in %s", nm);
    	return 0;
    }
    for (i = 0; i < num; i++)
    {
    	if ((rs = sscanf(pp[i], "%d", &tmp)) < 1)
    	{
	    log(LOG_FATAL, "Error in resource %s: %s", r, pp[i]);
	    return 0;
	}
	new->types[i].max_keys = tmp;
    }
    return new;
}

/*
 * Close isam file.
 */
int is_close(ISAM is)
{
    log(LOG_DEBUG, "is_close()");
    log(LOG_LOG, "is_close needs to close individual files.");
    xfree(is);
    return 0;
}

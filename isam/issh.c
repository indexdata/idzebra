/* $Id: issh.c,v 1.6 2002-08-02 19:26:56 adam Exp $
   Copyright (C) 1995,1996,1997,1998,1999,2000,2001,2002
   Index Data Aps

This file is part of the Zebra server.

Zebra is free software; you can redistribute it and/or modify it under
the terms of the GNU General Public License as published by the Free
Software Foundation; either version 2, or (at your option) any later
version.

Zebra is distributed in the hope that it will be useful, but WITHOUT ANY
WARRANTY; without even the implied warranty of MERCHANTABILITY or
FITNESS FOR A PARTICULAR PURPOSE.  See the GNU General Public License
for more details.

You should have received a copy of the GNU General Public License
along with Zebra; see the file LICENSE.zebra.  If not, write to the
Free Software Foundation, 59 Temple Place - Suite 330, Boston, MA
02111-1307, USA.
*/



#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <assert.h>

#include <isam.h>

static ISAM is = 0;

int c_open(char *p);
int c_close(char *p);
int c_write(char *p);
int c_merge(char *p);
int c_rewind(char *p);
int c_read(char *p);
int c_exit(char *p);
int c_help(char *p);
int c_list(char *p);
int c_pos(char *p);

struct
{
    char *word;
    int (*fun)(char *p);
} cmdl[] =
{
    {"open", c_open},
    {"close", c_close},
    {"write", c_write},
    {"merge", c_merge},
    {"rewind", c_rewind},
    {"read", c_read},
    {"exit", c_exit},
    {"quit", c_exit},
    {"help", c_help},
    {"list", 0},
    {"pos", c_pos},

    {0,0}
};


int c_pos(char *p)
{
}

int c_open(char *p)
{
    if (!*p)
    {
    	printf("Usage: open name\n");
    	return -1;
    }
    if (!(is = is_open(p, 1)))
    	return -1;
    return 0;
}

int c_close(char *p)
{
    if (!is)
    {
    	printf("Open first.\n");
    	return -1;
    }
    is_close(is);
    return 0;
}

int c_write(char *p) {}

int c_merge(char *p)
{
    char line[100], buf[1024], ch;
    int pos = 0, num = 0;
    int op, key;
    int val;

    if (!is)
    {
    	printf("Open first.\n");
    	return -1;
    }
    if (!sscanf(p, "%d", &val))
    {
    	printf("Usage: merge <address>\n");
    	return -1;
    }
    printf("Enter pairs of <operation> <key>. Terminate with '.'\n");
    while (gets(line) && *line != '.')
    {
    	if (sscanf(line, "%d %d", &op, &key) < 2)
    	{
	    printf("Error in input\n");
	    return -1;
	}
	ch = op;
	*(buf + pos++) = ch;
	memcpy(buf + pos, &key, sizeof(key));
	pos += sizeof(key);
	num++;
    }
    printf("is_merge(-->%d) returns: %d\n", val, is_merge(is, val, num , buf));
    return 0;
}

int c_rewind(char *p)
{
    if (!is)
    {
    	printf("Open first.\n");
    	return -1;
    }
}

int c_read(char *p) {}

int c_exit(char *p)
{
    exit(0);
}

int c_help(char *p)
{
    int i;

    printf("ISSH: ISAM debugging shell.\n");
    printf("Commands:\n");
    for (i = 0; cmdl[i].word; i++)
	printf("    %s %s\n", cmdl[i].word, cmdl[i].fun ? "": "UNDEFINED");
    return 0;
}

int main()
{
    char line[1024];
    static char word[1024] = "help", arg[1024] = "";
    int i;

    log_init(LOG_ALL, "issh", 0);

    common_resource = res_open("testres");
    assert(common_resource);

    for (;;)
    {
    	printf("ISSH> ");
    	fflush(stdout);
    	if (!gets(line))
	    return 0;
	*arg = '\0';
	if (*line && sscanf(line, "%s %[^;]", word, arg) < 1)
	    abort();
	for (i = 0; cmdl[i].word; i++)
	    if (!strncmp(cmdl[i].word, word, strlen(word)))
	    {
	    	printf("%s\n", (*cmdl[i].fun)(arg) == 0 ? "OK" : "FAILED");
	    	break;
	    }
	if (!cmdl[i].word)
	    (*c_help)("");
    }
}

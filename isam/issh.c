/*
 * Excersizer-application for the isam subsystem. Don't play with it.
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

    assert(common_resource = res_open("testres"));


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

/* $Id: zebrash.c,v 1.30 2004-11-19 10:27:04 heikki Exp $
   Copyright (C) 2002,2003,2004
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

/* 
   zebrash.c - command-line interface to zebra API
*/

#include <stdio.h>
#include <stdlib.h>
#include <string.h> 
#include <ctype.h>
#include <unistd.h>  /* for isatty */

#if HAVE_READLINE_READLINE_H
#include <readline/readline.h> 
#endif
#if HAVE_READLINE_HISTORY_H
#include <readline/history.h>
#endif

#include <idzebra/api.h>
#include <yaz/ylog.h>
#include <yaz/proto.h>
#include <yaz/sortspec.h>
#include <yaz/options.h>
#include <yaz/wrbuf.h>

#define MAX_NO_ARGS 32
#define MAX_OUT_BUFF 4096
#define MAX_ARG_LEN 1024
#define PROMPT "ZebraSh>"
#define DEFAULTCONFIG "./zebra.cfg"
#define DEFAULTDATABASE "Default"
#define DEFAULTRESULTSET "MyResultSet"


/**************************************
 * Global variables (yuck!)
 */

ZebraService zs=0;  /* our global handle to zebra */
ZebraHandle  zh=0;  /* the current session */
/* time being, only one session works */
int nextrecno=1;  /* record number to show next */
static char *default_config = DEFAULTCONFIG;
static int log_level=0;


/**************************************
 * Help functions
 */

 
static int split_args( char *line, char** args )
{ /* splits line into individual null-terminated strings, 
   * returns pointers to them in args */
  /* FIXME - do we need to handle quoted args ?? */
    char *p=line;
    int i=0;
    int n=0;
    args[0]=0; /* by default */
    while (*p==' ' || *p=='\t' || *p=='\n')
	    p++;
    while (*p)
    {
	while (*p==' ' || *p=='\t' || *p=='\n')
	    p++;
	if (*p=='#')  /* skip comments */
	    break;  
	args[i++]=p;
	args[i]=0;
	while (*p && *p!=' ' && *p!='\t' && *p!='\n' && *p!='#')
	    p++;
	*p++='\0';
    }
    n=i;
    while (n<MAX_NO_ARGS)
	args[n++]=0;
    return i;
}

static char *defarg( char *arg, char *def )
{
    if (!arg)
	return def;
    if (!*arg)
	return def;
    return arg;
}
static int defargint( char *arg, int def )
{
    int v=def;
    char *a=defarg(arg,0);
    if (a)
	sscanf(a," %i", &v);
    return v;
}

static char *restargs( char *args[], int n)
{ /* Returns the rest of the arguments, starting at the nth, */
  /* to the end of the command line. Assumes args[0] contains */
  /* the original line, minus the command itself */
    int skiplen= args[n]-args[1];
    if (skiplen > strlen(args[0]))
        return "";
    return args[0]+skiplen;
}

int onecommand( char *line, WRBUF outbuff, const char *prevout); 

/**************************************
 * Simple support commands
 */

int cmd_echo( char *args[], WRBUF outbuff)
{
    wrbuf_printf(outbuff,"%s\n",restargs(args,1));
    return 0;
}
 
int cmd_quit( char *args[], WRBUF outbuff)
{
    if (zs)
    {
        onecommand("zebra_close",outbuff,"");
        zs=0;
    }
    if (zh)
    {
        onecommand("zebra_stop",outbuff,"");
        zh=0;
    }
    wrbuf_puts(outbuff, "bye");
    return -99; /* special stop signal */
}

/**************************************
 * Tests for starting and stopping zebra, etc
 */
 
static int cmd_help( char *args[], WRBUF outbuff); 
 
static int cmd_zebra_start( char *args[], WRBUF outbuff)
{
    char *conf=args[1];
    if (!conf || !*conf) {
	wrbuf_puts(outbuff,"no config file specified, using ");
	wrbuf_puts(outbuff, default_config);
	wrbuf_puts(outbuff, "\n");
	conf=default_config;
    }
    zs=zebra_start(conf);
    if (!zs) {
	wrbuf_puts(outbuff, "zebra_start failed" );
	return 2;
    }
    return 0; /* ok */
}
 
static int cmd_zebra_stop( char *args[], WRBUF outbuff)
{
    if (!zs)
	wrbuf_puts(outbuff,"zebra seems not to have been started, "
		   "stopping anyway\n");
    zebra_stop(zs);
    zs=0;
    return 0; /* ok */
}

static int cmd_zebra_open( char *args[], WRBUF outbuff)
{
    if (!zs)
	wrbuf_puts(outbuff,"zebra seems not to have been started, "
		   "trying anyway\n");
    zh=zebra_open(zs);
    return 0; /* ok */
}

static int cmd_zebra_close( char *args[], WRBUF outbuff)
{
    if (!zh)
	wrbuf_puts(outbuff,"Seems like you have not called zebra_open,"
		   "trying anyway\n");
    zebra_close(zh);
    return 0; /* ok */
}

static int cmd_quickstart( char *args[], WRBUF outbuff)
{
    char tmp[128];
    int rc=0;
    if (!rc)
        rc=onecommand("yaz_log_file zebrash.log",outbuff,"");
    if (!rc)
        rc=onecommand("yaz_log_prefix ZebraSh", outbuff,"");
    sprintf(tmp, "yaz_log_level 0x%x", YLOG_DEFAULT_LEVEL | log_level );
    if (!rc)
        rc=onecommand(tmp,outbuff,"");
    yaz_log(log_level,"quickstart");
    if (!zs)
        if (!rc)
            rc=onecommand("zebra_start",outbuff,"");
    if (!zh)
        if (!rc)
            rc=onecommand("zebra_open",outbuff,"");
    if (!rc)
        rc=onecommand("select_database Default",outbuff,"");
    return rc;
}

/**************************************
 * Log file handling
 */

static int cmd_yaz_log_file( char *args[], WRBUF outbuff)
{
    char *fn = defarg(args[1],0);
    wrbuf_printf(outbuff, "sending yaz-log to %s\n",fn);
    yaz_log_init_file(fn);
    return 0; /* ok */
}

static int cmd_yaz_log_level( char *args[], WRBUF outbuff)
{
    int  lev = defargint(args[1],YLOG_DEFAULT_LEVEL);
    wrbuf_printf(outbuff, "setting yaz-log to level %d (ox%x)\n",lev,lev);
    yaz_log_init_level(lev);
    return 0; /* ok */
}

static int cmd_yaz_log_prefix( char *args[], WRBUF outbuff)
{
    char *pref = defarg(args[1],"ZebraSh");
    wrbuf_printf(outbuff, "setting yaz-log prefix to %s\n",pref);
    yaz_log_init_prefix(pref);
    return 0; /* ok */
}

static int cmd_logf( char *args[], WRBUF outbuff)
{
    int lev = defargint(args[1],0);
    int i=1;  
    if (lev)
	i=2;
    else
	lev=YLOG_LOG; /* this is in the default set!*/
    yaz_log( lev, restargs(args,i));
    return 0; /* ok */
}
 
/****************
 * Error handling 
 */
static int cmd_err ( char *args[], WRBUF outbuff)
{
    wrbuf_printf(outbuff, "errCode: %d \nerrStr:  %s\nerrAdd:  %s \n",
		 zebra_errCode (zh),
		 zebra_errString (zh),  
		 zebra_errAdd (zh) );
    return 0; /* ok */
}
static int cmd_errcode ( char *args[], WRBUF outbuff)
{
    wrbuf_printf(outbuff, "errCode: %d \n",
		 zebra_errCode (zh));
    return 0; /* ok */
}
static int cmd_errstr ( char *args[], WRBUF outbuff)
{
    wrbuf_printf(outbuff, "errStr:  %s\n",
		 zebra_errString (zh));
    return 0; /* ok */
}
static int cmd_erradd ( char *args[], WRBUF outbuff)
{
    wrbuf_printf(outbuff, "errAdd:  %s \n",
		 zebra_errAdd (zh) ); 
    return 0; /* ok */
}

/**************************************
 * Admin commands
 */

static int cmd_init ( char *args[], WRBUF outbuff)
{
    zebra_init(zh);
    return 0; /* ok */
}

static int cmd_select_database ( char *args[], WRBUF outbuff)
{
    char *db=defarg(args[1],DEFAULTDATABASE);
	wrbuf_printf(outbuff,"Selecting database '%s'\n",db);
    return zebra_select_database(zh, db);
}
 
static int cmd_create_database( char *args[], WRBUF outbuff)
{
    char *db=defarg(args[1],DEFAULTDATABASE);
    wrbuf_printf(outbuff,"Creating database '%s'\n",db);
    
    return zebra_create_database(zh, db);
}

static int cmd_drop_database( char *args[], WRBUF outbuff)
{
    char *db=args[1];
    if (!db)
        db="Default";
    wrbuf_printf(outbuff,"Dropping database '%s'\n",db);
    return zebra_drop_database(zh, db);
}

static int cmd_begin_trans( char *args[], WRBUF outbuff)
{
    int rw=0;
    if (args[1] && ( (args[1][0]=='1') || (args[1][0]=='w') ))
        rw=1;
    return zebra_begin_trans(zh,rw);
}

static int cmd_end_trans( char *args[], WRBUF outbuff)
{
    return zebra_end_trans(zh);
}
/*************************************
 * Inserting and deleting
 */

static int cmd_record_insert( char *args[], WRBUF outbuff)
{
    SYSNO sysno=0;
    int rc;
    char *rec=restargs(args,1);
    
    rc = zebra_insert_record(zh,
			     0,  /* record type */
			     &sysno,
			     0,  /* match */
			     0,  /* fname */
			     rec,
			     strlen(rec),
			     0);
    if (0==rc)
    {
        wrbuf_printf(outbuff,"ok sysno=%d\n",sysno);
    }
    return rc;
}


static int cmd_exchange_record( char *args[], WRBUF outbuff)
{
    char *id = args[1];
    char *action = args[2];
    int rc;
    char *rec=restargs(args,3);
    if (!(id && action && args[4] ))
    {
	wrbuf_puts(outbuff,"Missing arguments!\n");
	onecommand("help exchange_record", outbuff, "");
	return -90;
    }
    rc=zebra_admin_exchange_record(zh, rec, strlen(rec),
        id, strlen(id), atoi(action));
    return rc;
}

/**********************************
 * Searching and retrieving
 */

static int cmd_search_pqf( char *args[], WRBUF outbuff)
{
    int hits=0;
    char *set=args[1];
    char *qry=restargs(args,2);
    int rc;
    rc=zebra_search_PQF(zh, qry, set, &hits);
    if (0==rc)
        wrbuf_printf(outbuff,"%d hits found\n",hits);
    return rc;
}

static int cmd_find( char *args[], WRBUF outbuff)
{
    char *setname=DEFAULTRESULTSET;
    int rc;
    int hits=0;
    WRBUF qry=wrbuf_alloc();
    if (0==strstr(args[0],"@attr"))
        wrbuf_puts(qry, "@attr 1=/ ");
    wrbuf_puts(qry,restargs(args,1));
    if (!zh)
	onecommand("quickstart", outbuff, "");
    wrbuf_printf(outbuff, "find %s\n",wrbuf_buf(qry));
    rc=zebra_search_PQF(zh, wrbuf_buf(qry), setname, &hits);
    if (0==rc)
    {
        wrbuf_printf(outbuff,"%d hits found\n",hits);
        nextrecno=1;
    }
    wrbuf_free(qry,1);
    return rc;
}

static int cmd_show( char *args[], WRBUF outbuff)
{
    int start=defargint(args[1], nextrecno);
    int nrecs=defargint(args[2],1);
    char *setname=defarg(args[3],DEFAULTRESULTSET);
    int rc=0;
    ZebraRetrievalRecord *recs;
    ODR odr;
    Z_RecordComposition *pcomp=0;
    int i;
    oid_value format;

    odr=odr_createmem(ODR_ENCODE);
    recs= odr_malloc(odr,sizeof(ZebraRetrievalRecord)*nrecs);
    rc =z_RecordComposition(odr, &pcomp, 0,"recordComposition");
    format=oid_getvalbyname ("xml"); /*FIXME - let the user specify*/
    for (i=0;i<nrecs;i++)
        recs[i].position=start+i;

    rc = zebra_records_retrieve (zh, odr, setname,
				 pcomp, format, nrecs,recs);
    if (0==rc)
    {
        for (i=0;i<nrecs;i++)
        {
            printf("Err %d: %d\n",i,recs[i].errCode);
            if (recs[i].buf)
            {
                wrbuf_printf(outbuff,"Record %d\n", recs[i].position);
                wrbuf_write(outbuff, recs[i].buf, recs[i].len);
                wrbuf_puts(outbuff, "\n");
            } else
                wrbuf_printf(outbuff,"NO Record %d\n", recs[i].position);
        }
        nextrecno=start+nrecs;
    }
    odr_destroy(odr);
    return rc;
} /* cmd_show */

static int cmd_sort( char *args[], WRBUF outbuff)
{
    int rc=0;
    ODR odr;
    int sortstatus=0;
    Z_SortKeySpecList *spec=0;
    const char * inpsets[]={ DEFAULTRESULTSET, 0};
    /* FIXME - allow the user to specify result sets in/out */

    odr=odr_createmem(ODR_ENCODE);
    spec=yaz_sort_spec (odr, restargs(args,1));
    if (!spec)
        rc=1;
    if (!rc)
        rc=zebra_sort(zh, odr,
                        1, inpsets,
                        DEFAULTRESULTSET,
                        spec,
                        &sortstatus);
    if (!rc)
        wrbuf_printf(outbuff, "sort returned status %d\n",sortstatus);

    odr_destroy(odr);
    return rc;
} /* cmd_sort */
/*
 *
 * int bend_sort (void *handle, bend_sort_rr *rr)
 * {
 *     ZebraHandle zh = (ZebraHandle) handle;
 *
 *     zebra_sort (zh, rr->stream,
 *                     rr->num_input_setnames, (const char **)
 *                     rr->input_setnames,
 *                     rr->output_setname,
 *                     rr->sort_sequence,
 *                     &rr->sort_status);
 *     zebra_result (zh, &rr->errcode,
 *                  &rr->errstring);
 *     return 0;
 *  }
 *
 */

/**************************************)
 * Command table, parser, and help 
 */

struct cmdstruct
{
    char *cmd;
    char *args;
    char *explanation;
    int (*testfunc)(char *args[], WRBUF outbuff);
} ;

 
struct cmdstruct cmds[] = {
    /* special cases:
     *   if text is 0, does not list the command
     *   if cmd is "", adds the args (and newline) in command listing
     */
    { "", "Starting and stopping:", "", 0 },
    { "zebra_start", 
      "[configfile]", 
      "starts the zebra service. You need to call this first\n"
      "if no configfile is given, assumes " DEFAULTCONFIG, 
      cmd_zebra_start },
    { "zebra_stop",   "", 
      "stops the zebra service", 
      cmd_zebra_stop },
    { "zebra_open", "",  
      "starts a zebra session. Once you have called zebra_start\n"
      "you can call zebra_open to start working", 
      cmd_zebra_open },
    { "zebra_close", "", 
      "closes a zebra session", 
      cmd_zebra_close }, 
    { "quickstart", "[configfile]", 
      "Does a zebra_start, zebra_open, and sets up the log", 
      cmd_quickstart }, 
  
    { "", "Log file:","", 0},  
    { "yaz_log_file", 
      "[filename]",
      "Directs the log to filename (or stderr)",
      cmd_yaz_log_file },
    { "yaz_log_level", 
      "[level]",
      "Sets the logging level (or returns to default)",
      cmd_yaz_log_level },
    { "yaz_log_prefix", 
      "[prefix]",
      "Sets the log prefix",
      cmd_yaz_log_prefix},    
    { "yaz_log", 
      "[level] text...",
      "writes an entry in the log",
      cmd_logf},    

    { "", "Error handling:","", 0},
    { "err",  "",
      "Displays zebra's error status (code, str, add)",
      cmd_err},    
    { "errcode",  "",
      "Displays zebra's error code",
      cmd_errcode},    
    { "errstr",  "",
      "Displays zebra's error string",
      cmd_errstr},    
    { "erradd",  "",
      "Displays zebra's additional error message",
      cmd_erradd},    
  
    { "", "Admin:","", 0}, 
    { "init",  "",
      "Initializes the zebra database, destroying all data in it",
      cmd_init},    
    { "select_database",  "basename",
      "Selects a database",
      cmd_select_database},    
    { "create_database", "basename",
      "Create database",
      cmd_create_database},
    { "drop_database", "basename",
      "Drop database",
      cmd_drop_database},
    { "begin_trans", "[rw]",
      "Begins a transaction. rw=1 means write, otherwise read-only",
      cmd_begin_trans},
    { "end_trans","",
      "Ends a transaction",
      cmd_end_trans},

    { "","Updating:","",0},
    { "record_insert","record",
      "inserts an sgml record into Default",
      cmd_record_insert},
    { "exchange_record","database record-id action record",
      "inserts (1), updates (2), or deletes (3) a record \n"
      "record-id must be a unique identifier for the record",
      cmd_exchange_record},

    { "","Searching and retrieving:","",0},
    { "search_pqf","setname query",
      "search ",
      cmd_search_pqf},
    { "find","query",
      "simplified search",
      cmd_find},
    { "f","query",
      "simplified search",
      cmd_find},
    { "show","[start] [numrecs] [resultset]",
      "shows a result",
      cmd_show},
    { "s","[start] [numrecs] [resultset]",
      "shows a result",
      cmd_show},
    { "sort","sortspec",
      "sorts a result set. (example spec: 1=4 >)",
      cmd_sort},
      
    { "", "Misc:","", 0}, 
    { "echo", "string", 
      "ouputs the string", 
      cmd_echo },
    { "q", "", 
      "exits the program", 
      cmd_quit },
    { "quit", "", 
      "exits the program", 
      cmd_quit },
    { "help", "[command]", 
      "Gives help on command, or lists them all", 
      cmd_help },
    { "", "help [command] gives more info on command", "",0 },   
  
    {0,0,0,0} /* end marker */
};
 
int onecommand( 
		char *line,     /* input line */
		WRBUF outbuff,  /* output goes here */
		const char *prevout) /* prev output, for 'expect' */
{
    int i;
    char *args[MAX_NO_ARGS];
    int nargs;
    char argbuf[MAX_ARG_LEN];
    yaz_log(log_level,"%s",line);
    strncpy(argbuf,line, MAX_ARG_LEN-1);
    argbuf[MAX_ARG_LEN-1]='\0'; /* just to be sure */
    /*memset(args,'\0',MAX_NO_ARGS*sizeof(char *));*/
    nargs=split_args(argbuf, args);
    
#if 0
    for (i = 0; i <= n; i++)
    {
	const char *cp = args[i];
	printf ("args %d :%s:\n", i, cp ? cp : "<null>");
    }
#endif
    if (0==nargs)
	    return -90; /* no command on line, too bad */

    if (0==strcmp(args[0],"expect")) 
    {
	char *rest;
        if (nargs>1) /* args[0] is not yet set, can't use restargs */
            rest= line + (args[1]-argbuf); /* rest of the line */
        else
            return -1; /* need something to expect */
	if (0==strstr(prevout,rest))
	{
	    printf( "Failed expectation, '%s' not found\n", rest);
            exit(9); 
	}
	return 0;
    }
    for (i=0;cmds[i].cmd;i++)
	if (0==strcmp(cmds[i].cmd, args[0])) 
	{
	    if (nargs>1)
		args[0]= line + (args[1]-argbuf); /* rest of the line */
	    else
		args[0]=""; 
	    return ((cmds[i].testfunc)(args,outbuff));
	}
    wrbuf_printf(outbuff, "Unknown command '%s'. Try help\n",args[0]);
    yaz_log(log_level,"Unknown command");
    return -90; 
}
 
static int cmd_help( char *args[], WRBUF outbuff)
{ 
    int i;
    int linelen;
    if (args[1]) 
    { /* help for a single command */ 
	for (i=0;cmds[i].cmd;i++)
	    if (0==strcmp(cmds[i].cmd, args[1])) 
	    {
                wrbuf_printf(outbuff,"%s  %s\n%s\n",
		             cmds[i].cmd, cmds[i].args, 
			     cmds[i].explanation);
		return 0;
	    }
	wrbuf_printf(outbuff, "Unknown command '%s'", args[1]);
    }
    else 
    { /* list all commands */
        linelen=9999;
	for (i=0;cmds[i].cmd;i++)
        {
            if (*cmds[i].cmd)
            { /* ordinary command */
                if (linelen>50)
                {
                    wrbuf_puts(outbuff,"\n   ");
                    linelen=0;
                }
                linelen += strlen(cmds[i].cmd) + 2;
                wrbuf_printf(outbuff,"%s ", cmds[i].cmd);
            } else
            { /* section head */
                wrbuf_printf(outbuff,"\n%s\n   ",cmds[i].args);
                linelen=0;
            }
	    } /* for */
        wrbuf_puts(outbuff,"\n");
    }
    return 0;
}
 
/* If Zebra reports an error after an operation,
 * append it to the outbuff and log it */
static void Zerrors ( WRBUF outbuff)
{
    int ec;
    if (!zh)
	return ;
    ec=zebra_errCode (zh);
    if (ec)
    {
	yaz_log(log_level, "   Zebra error %d: %s, (%s)",
	     ec, zebra_errString (zh),
	     zebra_errAdd (zh) );
	wrbuf_printf(outbuff, "   Zebra error %d: %s, (%s)\n",
		     ec, zebra_errString (zh),
		     zebra_errAdd (zh) );
	zebra_clearError(zh);
    }
}

/************************************** 
 * The shell
 */
 
void shell()
{
    int rc=0;
    WRBUF outbuff=wrbuf_alloc();
    char prevout[MAX_OUT_BUFF]=""; /* previous output for 'expect' */
    wrbuf_puts(outbuff,"Zebrash at your service");
    while (rc!=-99)
    {
	char *nl_cp;
	char buf[MAX_ARG_LEN];
	char* line_in = 0;
#if HAVE_READLINE_READLINE_H
	if (isatty(0)) {
	    line_in=readline(PROMPT);
	    if (!line_in)
		break;
#if HAVE_READLINE_HISTORY_H
	    if (*line_in)
		add_history(line_in);
#endif
	}
#endif
	/* line_in != NULL if readine is present and input is a tty */
	
	printf (PROMPT); 
	fflush (stdout);
	if (line_in)
	{
	    if(strlen(line_in) > MAX_ARG_LEN-1) {
		fprintf(stderr,"Input line too long\n");
		break;
	    }
	    strcpy(buf,line_in);
	    free (line_in);
	}
	else 
	{
	    if (!fgets (buf, MAX_ARG_LEN-1, stdin))
		break; 
	}
	
	/* get rid of \n in line */
	if ((nl_cp = strchr(buf, '\n')))
	    *nl_cp = '\0';
	strncpy(prevout, wrbuf_buf(outbuff), MAX_OUT_BUFF);
        wrbuf_rewind(outbuff);
	rc=onecommand(buf, outbuff, prevout);
	if (rc==0)
	{
	    wrbuf_puts(outbuff, "   OK\n");
	    yaz_log(log_level, "OK");
	}
	else if (rc>-90)
	{
	    wrbuf_printf(outbuff, "   command returned %d\n",rc);
	} 
	Zerrors(outbuff);
	printf("%s\n", wrbuf_buf(outbuff));
    } /* while */
    wrbuf_free(outbuff,1);
} /* shell() */


static void usage()
{
    printf ("usage:\n");
    printf ("zebrash [-c config]\n");
    exit(1);
}
/**************************************
 * Main 
 */

int main (int argc, char ** argv)
{
    int ret;
    char *arg = 0;
    while ((ret = options ("c:h", argv, argc, &arg)) != -2)
    {
	switch(ret)
	{
	case 'c':
	    default_config = arg;
	    break;
	case 'h':
	    usage();
        /* FIXME - handle -v */
	default:
	    fprintf(stderr, "bad option %s\n", arg);
	    usage();
	}
    }
    log_level=yaz_log_module_level("zebrash");

    shell();
    return 0;
} /* main */

/* $Id: zebrash.c,v 1.16 2003-07-03 16:16:22 heikki Exp $
   Copyright (C) 2002,2003
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

#if HAVE_READLINE_READLINE_H
#include <readline/readline.h> 
#endif
#if HAVE_READLINE_HISTORY_H
#include <readline/history.h>
#endif

#include "zebraapi.h"
#include <yaz/log.h>
#include <yaz/proto.h>

#define MAX_NO_ARGS 32
#define MAX_OUT_BUFF 4096
#define MAX_ARG_LEN 1024
#define PROMPT "ZebraSh>"
#define DEFAULTCONFIG "./zebra.cfg"
#define DEFAULTRESULTSET "MyResultSet"

/**************************************
 * Global variables (yuck!)
 */

ZebraService zs=0;  /* our global handle to zebra */
ZebraHandle  zh=0;  /* the current session */
/* time being, only one session works */
int nextrecno=0;  /* record number to show next */

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

int onecommand( char *line, char *outbuff, const char *prevout); 
 
/**************************************
 * Simple support commands
 */

int cmd_echo( char *args[], char *outbuff)
{
    strcpy(outbuff, args[0]);
    return 0;
}
 
int cmd_quit( char *args[], char *outbuff)
{
    strcpy(outbuff, "bye");
    return -99; /* special stop signal */
}

/**************************************
 * Tests for starting and stopping zebra, etc
 */
 
static int cmd_help( char *args[], char *outbuff); 
 
static int cmd_zebra_start( char *args[], char *outbuff)
{
    char *conf=args[1];
    if (!conf || !*conf) {
	strcat(outbuff,"no config file specified, using "
	       DEFAULTCONFIG "\n" );
	conf=DEFAULTCONFIG;
    }
    zs=zebra_start(conf);
    if (!zs) {
	strcpy(outbuff, "zebra_start failed" );
	return 2;
    }
    return 0; /* ok */
}
 
static int cmd_zebra_stop( char *args[], char *outbuff)
{
    if (!zs)
	strcat(outbuff,"zebra seems not to have been started, "
	       "stopping anyway\n");
    zebra_stop(zs);
    zs=0;
    return 0; /* ok */
}

static int cmd_zebra_open( char *args[], char *outbuff)
{
    if (!zs)
	strcat(outbuff,"zebra seems not to have been started, "
	       "trying anyway\n");
    zh=zebra_open(zs);
    return 0; /* ok */
}

static int cmd_zebra_close( char *args[], char *outbuff)
{
    if (!zh)
	strcat(outbuff,"Seems like you have not called zebra_open,"
	       "trying anyway\n");
    zebra_close(zh);
    return 0; /* ok */
}

static int cmd_quickstart( char *args[], char *outbuff)
{
    char tmp[128];
    int rc=0;
    if (!rc)
        rc=onecommand("yaz_log_file zebrash.log",outbuff,"");
    if (!rc)
        rc=onecommand("yaz_log_prefix ZebraSh", outbuff,"");
    sprintf(tmp, "yaz_log_level 0x%x", LOG_DEFAULT_LEVEL | LOG_APP);
    if (!rc)
        rc=onecommand(tmp,outbuff,"");
    logf(LOG_APP,"quickstart");
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

static int cmd_yaz_log_file( char *args[], char *outbuff)
{
    char *fn = defarg(args[1],0);
    char tmp[255];
    sprintf(tmp, "sending yaz-log to %s\n",fn);
    strcat(outbuff, tmp);
    yaz_log_init_file(fn);
    return 0; /* ok */
}

static int cmd_yaz_log_level( char *args[], char *outbuff)
{
    int  lev = defargint(args[1],LOG_DEFAULT_LEVEL);
    char tmp[255];
    sprintf(tmp, "setting yaz-log to level %d (ox%x)\n",lev,lev);
    strcat(outbuff, tmp);
    yaz_log_init_level(lev);
    return 0; /* ok */
}

static int cmd_yaz_log_prefix( char *args[], char *outbuff)
{
    char *pref = defarg(args[1],"ZebraSh");
    char tmp[255];
    sprintf(tmp, "setting yaz-log prefix to %s\n",pref);
    strcat(outbuff, tmp);
    yaz_log_init_prefix(pref);
    return 0; /* ok */
}

static int cmd_logf( char *args[], char *outbuff)
{
    int lev = defargint(args[1],0);
    char tmp[MAX_OUT_BUFF]="";
    int i=1;
    if (lev)
	i=2;
    else
	lev=LOG_LOG; /* this is in the default set!*/
    while (args[i])
    {
	strcat(tmp, args[i++]);
	strcat(tmp, " ");
    }
    logf(lev,tmp);
    return 0; /* ok */
}
 
/****************
 * Error handling 
 */
static int cmd_err ( char *args[], char *outbuff)
{
    char tmp[MAX_OUT_BUFF];
    sprintf(tmp, "errCode: %d \nerrStr:  %s\nerrAdd:  %s \n",
	    zebra_errCode (zh),
	    zebra_errString (zh),  
	    zebra_errAdd (zh) );
    strcat(outbuff, tmp);
    return 0; /* ok */
}
static int cmd_errcode ( char *args[], char *outbuff)
{
    char tmp[MAX_OUT_BUFF];
    sprintf(tmp, "errCode: %d \n",
	    zebra_errCode (zh));
    strcat(outbuff, tmp);
    return 0; /* ok */
}
static int cmd_errstr ( char *args[], char *outbuff)
{
    char tmp[MAX_OUT_BUFF];
    sprintf(tmp, "errStr:  %s\n",
	    zebra_errString (zh));
    strcat(outbuff, tmp);
    return 0; /* ok */
}
static int cmd_erradd ( char *args[], char *outbuff)
{
    char tmp[MAX_OUT_BUFF];
    sprintf(tmp, "errAdd:  %s \n",
	    zebra_errAdd (zh) ); 
    strcat(outbuff, tmp);
    return 0; /* ok */
}

/**************************************
 * Admin commands
 */

static int cmd_init ( char *args[], char *outbuff)
{
    zebra_init(zh);
    return 0; /* ok */
}

static int cmd_select_database ( char *args[], char *outbuff)
{
    char *db=args[1];
    if (!db) {
        db="Default";
	strcat(outbuff,"Selecting database 'Default'\n");
    }
    return zebra_select_database(zh, db);
}
 
static int cmd_create_database( char *args[], char *outbuff)
{
    char *db=args[1];
    if (!db)
        db="Default";
    strcat(outbuff,"Creating database ");
    strcat(outbuff,db);
    strcat(outbuff,"\n");
	
    return zebra_create_database(zh, db);
}

static int cmd_drop_database( char *args[], char *outbuff)
{
    char *db=args[1];
    if (!db)
        db="Default";
    strcat(outbuff,"Dropping database ");
    strcat(outbuff,db);
    strcat(outbuff,"\n");
	
    return zebra_drop_database(zh, db);
}

static int cmd_begin_trans( char *args[], char *outbuff)
{
    int rw=0;
    if (args[1] && ( (args[1][0]=='1') || (args[1][0]=='w') ))
        rw=1;
    return zebra_begin_trans(zh,rw);
}
static int cmd_end_trans( char *args[], char *outbuff)
{
    return zebra_end_trans(zh);
}
/*************************************
 * Inserting and deleting
 */

static int cmd_record_insert( char *args[], char *outbuff)
{
    int sysno=0;
    char buf[MAX_ARG_LEN];
    int i;
    int rc;
    
    i=1;
    buf[0]='\0';
    while (args[i])
    {
	strcat(buf, args[i++]);
	strcat(buf, " ");
    }
    rc=zebra_record_insert(zh,buf, strlen(buf), &sysno);
    if (0==rc)
    {
        sprintf(buf,"ok sysno=%d\n",sysno);
	strcat(outbuff,buf);
    }
    return rc;
}


static int cmd_exchange_record( char *args[], char *outbuff)
{
    char *base=args[1];
    char *id = args[2];
    char *action = args[3];
    int i=4;
    int rc;
    char buf[MAX_ARG_LEN];
    if (!(base && id && action && args[4] ))
    {
	strcat(outbuff,"Missing arguments!\n");
	onecommand("help exchange_record", outbuff, "");
	return -90;
    }
    while (args[i])
    {
	strcat(buf, args[i++]);
	strcat(buf, " ");
    }
    rc=zebra_admin_exchange_record(zh, base, buf, strlen(buf),
        id, strlen(id), atoi(action));
    return rc;
}

/**********************************
 * Searching and retrieving
 */

static int cmd_search_pqf( char *args[], char *outbuff)
{
    int hits=0;
    char *set=args[1];
    char qry[MAX_ARG_LEN]="";
    int i=2;
    int rc;
    while (args[i])
    {
	    strcat(qry, args[i++]);
	    strcat(qry, " ");
    }
    rc=zebra_search_PQF(zh, qry, set, &hits);
    if (0==rc)
    {
        sprintf(qry,"%d hits found\n",hits);
        strcat(outbuff,qry);
    }
    return rc;
}

static int cmd_find( char *args[], char *outbuff)
{
    char *setname=DEFAULTRESULTSET;
    char qry[MAX_ARG_LEN]="";
    int i=1;
    int rc;
    int hits=0;
    if (0==strstr(args[0],"@attr"))
        strcat(qry, "@attr 1=/ ");
    while (args[i])
    {
	    strcat(qry, args[i++]);
	    strcat(qry, " ");
    }
    if (!zh)
	    onecommand("quickstart", outbuff, "");
    strcat(outbuff, "find ");
    strcat(outbuff, qry);
    strcat(outbuff, "\n");
    rc=zebra_search_PQF(zh, qry, setname, &hits);
    if (0==rc)
    {
        sprintf(qry,"%d hits found\n",hits);
        strcat(outbuff,qry);
        nextrecno=0;
    }
    return rc;
}

static int cmd_show( char *args[], char *outbuff)
{
    int start=defargint(args[1], nextrecno);
    int nrecs=defargint(args[2],1);
    char *setname=defarg(args[3],DEFAULTRESULTSET);
    int rc=0;
    ODR odr;
    Z_RecordComposition *pcomp=0;

    oid_value format;
    ZebraRetrievalRecord recs;
    odr=odr_createmem(ODR_ENCODE);
    rc =z_RecordComposition(odr, &pcomp, 0,"recordComposition");
    printf("rc1=%d\n",rc);
    format=oid_getvalbyname ("xml"); /*FIXME*/

    rc = zebra_records_retrieve (zh, odr, setname,
            pcomp, format, nrecs, &recs);
                    
                    /*
                    ODR stream,
                    const char *setname, 
                    Z_RecordComposition *comp,
                    oid_value input_format,
                    int num_recs, 
                    ZebraRetrievalRecord *recs);
                    */

    nextrecno=start+1;
    return rc;
}
/**************************************)
 * Command table, parser, and help 
 */

struct cmdstruct
{
    char * cmd;
    char * args;
    char * explanation;
    int (*testfunc)(char *args[], char *outbuff);
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
    { "logf", 
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
		char *outbuff,  /* output goes here */
		const char *prevout) /* prev output, for 'expect' */
{
    int i;
    char *args[MAX_NO_ARGS];
    int nargs;
    char argbuf[MAX_ARG_LEN];
    logf(LOG_APP,"%s",line);
    strncpy(argbuf,line, MAX_ARG_LEN-1);
    argbuf[MAX_ARG_LEN-1]='\0'; /* just to be sure */
    memset(args,'\0',MAX_NO_ARGS*sizeof(char *));
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
        if (nargs>1)
            rest= line + (args[1]-argbuf); /* rest of the line */
        else
            return -1; /* need something to expect */
	printf("expecting '%s'\n",rest); /*!*/
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
    strcat(outbuff, "Unknown command '");
    strcat(outbuff,args[0] );
    strcat(outbuff,"'. Try help");
    logf(LOG_APP,"Unknown command");
    return -90; 
}
 
static int cmd_help( char *args[], char *outbuff)
{ 
    int i;
    char tmp[MAX_ARG_LEN];
    if (args[1]) 
    { /* help for a single command */ 
	for (i=0;cmds[i].cmd;i++)
	    if (0==strcmp(cmds[i].cmd, args[1])) 
	    {
		strcat(outbuff,cmds[i].cmd);
		strcat(outbuff,"  ");
		strcat(outbuff,cmds[i].args);
		strcat(outbuff,"\n");
		strcat(outbuff,cmds[i].explanation);
		strcat(outbuff,"\n");
		return 0;
	    }
	strcat(outbuff, "Unknown command ");
	strcat(outbuff, args[1] );
    }
    else 
    { /* list all commands */
	strcpy(tmp,"    ");
	for (i=0;cmds[i].cmd;i++)
	    if (cmds[i].explanation)
	    {
		/* sprintf(tmp, "%s %s %s\n",
		   cmds[i].cmd, cmds[i].args, cmds[i].explanation);
		*/
		strcat(tmp, cmds[i].cmd);
		strcat(tmp,"  ");
		if (!*cmds[i].cmd)
		{
		    strcat(outbuff, tmp);
		    strcat(outbuff,"\n");
		    strcpy(tmp,"    ");
		    if (*cmds[i].args)
		    {
			strcat(outbuff, cmds[i].args);
			strcat(outbuff,"\n");
		    }
		}
		if (strlen(tmp)>50)
		{
		    strcat(outbuff,tmp);
		    strcat(outbuff,"\n");
		    strcpy(tmp,"    ");
		}
	    }
	strcat(outbuff,tmp);
    }
    return 0;
}
 
/* If Zebra reports an error after an operation,
 * append it to the outbuff and log it */
static void Zerrors ( char *outbuff)
{
    int ec;
    char tmp[MAX_OUT_BUFF];
    if (!zh)
	return ;
    ec=zebra_errCode (zh);
    if (ec)
    {
	sprintf(tmp, "   Zebra error %d: %s, (%s)",
		ec, zebra_errString (zh),
		zebra_errAdd (zh) );
	strcat(outbuff, tmp);
	strcat(outbuff, "\n");
	logf(LOG_APP, tmp);
	zebra_clearError(zh);
    }
}
  
/************************************** 
 * The shell
 */
 
void shell()
{
    int rc=0;
    char tmp[MAX_ARG_LEN];
    char outbuff[MAX_OUT_BUFF]="";
    char prevout[MAX_OUT_BUFF]=""; /* previous output for 'expect' */
    while (rc!=-99)
    {
	char buf[MAX_ARG_LEN];
#if HAVE_READLINE_READLINE_H
	char* line_in;
	line_in=readline(PROMPT);
	if (!line_in)
	    break;
#if HAVE_READLINE_HISTORY_H
	if (*line_in)
	    add_history(line_in);
#endif
	if(strlen(line_in) > MAX_ARG_LEN-1) {
	    fprintf(stderr,"Input line too long\n");
	    break;
	};
	strcpy(buf,line_in);
	free (line_in);
#else    
	printf (PROMPT); 
	fflush (stdout);
	if (!fgets (buf, MAX_ARG_LEN-1, stdin))
	    break; 
#endif 
	
	strncpy(prevout, outbuff, MAX_OUT_BUFF);
	outbuff[0]='\0';
	rc=onecommand(buf, outbuff, prevout);
	if (rc==0)
	{
	    strcat(outbuff, "   OK\n");
	    logf(LOG_APP, "OK");
	}
	else if (rc>-90)
	{
	    sprintf(tmp, "   command returned %d\n",rc);
	    strcat(outbuff,tmp);
	} 
	Zerrors(outbuff);
  	printf("%s\n", outbuff);
    } /* while */
} /* shell() */
  
 
/**************************************
 * Main 
 */
 
int main (int argc, char ** args)
{
    shell();
    return 0;
} /* main */

/* zebrash.c - command-line interface to zebra API 
 *  $ID$
 *
 * Copyrigth 2003 Index Data Aps
 *
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

#define MAX_NO_ARGS 32
#define MAX_OUT_BUFF 4096
#define MAX_ARG_LEN 1024
#define PROMPT "ZebraSh>"
#define DEFAULTCONFIG "./zebra.cfg"

/**************************************
 * Global variables (yuck!)
 */

ZebraService zs=0;  /* our global handle to zebra */
ZebraHandle  zh=0;  /* the current session */
                  /* time being, only one session works */

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
                    "stopping anyway");
   zebra_stop(zs);
   zs=0;
   return 0; /* ok */
 }

static int cmd_zebra_open( char *args[], char *outbuff)
{
  if (!zs)
    strcat(outbuff,"zebra seems not to have been started, "
                   "trying anyway");
  zh=zebra_open(zs);
  return 0; /* ok */
}

static int cmd_zebra_close( char *args[], char *outbuff)
{
  if (!zh)
    strcat(outbuff,"Seems like you have not called zebra_open,"
                   "trying anyway");
  zebra_close(zh);
  return 0; /* ok */
}

static int cmd_quickstart( char *args[], char *outbuff)
{
  cmd_zebra_start(args,outbuff);
  cmd_zebra_open(args,outbuff);
  yaz_log_init_file("zebrash.log");
  yaz_log_init_prefix("ZebraSh");
  strcat(outbuff,"Started zebra, log in zebrash.log");
  return 0; /* ok */
}

/**************************************
 * Log file handling
 */

static int cmd_yaz_log_file( char *args[], char *outbuff)
{
  char *fn = defarg(args[1],0);
  char tmp[255];
  sprintf(tmp, "sending yaz-log to %s ",fn);
  strcat(outbuff, tmp);
  yaz_log_init_file(fn);
  return 0; /* ok */
}

static int cmd_yaz_log_level( char *args[], char *outbuff)
{
  int  lev = defargint(args[1],LOG_DEFAULT_LEVEL);
  char tmp[255];
  sprintf(tmp, "setting yaz-log to level %d (ox%x)",lev,lev);
  strcat(outbuff, tmp);
  yaz_log_init_level(lev);
  return 0; /* ok */
}

static int cmd_yaz_log_prefix( char *args[], char *outbuff)
{
  char *pref = defarg(args[1],"ZebraSh");
  char tmp[255];
  sprintf(tmp, "setting yaz-log prefix to %s",pref);
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
 
 
/**************************************
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
  { "yaz_log_file", 
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
  
  { "", "Misc:","", 0}, 
  { "echo", "string", 
    "ouputs the string", 
    cmd_echo },
  { "quit", "", 
    "exits the program", 
    cmd_quit },
  { "help", "[command]", 
    "Gives help on command, or lists them all", 
    cmd_help },
  { "", "help [command] gives more info on command", "",0 },   
  
  {0,0,0,0} /* end marker */
};
 
int onecommand( char *line, char *outbuff)
{
  int i;
  char *args[MAX_NO_ARGS];
  int n;
  char argbuf[MAX_ARG_LEN];
  strncpy(argbuf,line, MAX_ARG_LEN-1);
  argbuf[MAX_ARG_LEN-1]='\0'; /* just to be sure */
  n=split_args(argbuf, args);
  if (0==n)
    return 0; /* no command on line, too bad */
  for (i=0;cmds[i].cmd;i++)
    if (0==strcmp(cmds[i].cmd, args[0])) 
    {
      if (n>1)
        args[0]= line + (args[1]-argbuf); /* rest of the line */
      else
        args[0]=""; 
      return ((cmds[i].testfunc)(args,outbuff));
    }
  sprintf (outbuff, "Unknown command '%s'. Try help",args[0] );
  return -1; 
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
 
 
/************************************** 
 * The shell
 */
 
void shell()
{
  int rc=0;
  while (rc!=-99)
  {
    char buf[MAX_ARG_LEN];
    char outbuff[MAX_OUT_BUFF];
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
    outbuff[0]='\0';
    rc=onecommand(buf, outbuff);
  	printf("%s\n", outbuff);
  }

 }
 
 
/**************************************
 * Main 
 */
 
int main (int argc, char ** args)
{
  shell();
  return 0;
} /* main */

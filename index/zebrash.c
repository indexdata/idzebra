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

 
static int split_args( char *line, char** argv )
{ /* splits line into individual null-terminated strings, 
   * returns pointers to them in argv */
  char *p=line;
  int i=0;
  argv[0]=0; /* by default */
  while (*p==' ' || *p=='\t' || *p=='\n')
    p++;
  while (*p)
  {
    while (*p==' ' || *p=='\t' || *p=='\n')
      p++;
    argv[i++]=p;
    argv[i]=0;
    while (*p && *p!=' ' && *p!='\t' && *p!='\n')
      p++;
    *p++='\0';
  }
  return i;
}

 
 /**************************************
 * Individual commands
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
 
 int cmd_help( char *args[], char *outbuff); 
 
 int cmd_zebra_start( char *args[], char *outbuff)
 {
   char *conf=args[1];
   if (!conf || !*conf) {
     strcat(outbuff,"no config file specified, using "
                     DEFAULTCONFIG "\n" );
     conf=DEFAULTCONFIG;
   }
   zs=zebra_start(conf);
   if (!zs) {
     strcpy(outbuff, "zebra_open failed" );
     return 2;
   }
   return 0; /* ok */
 }
 
 int cmd_zebra_stop( char *args[], char *outbuff)
 {
   if (!zs)
     strcat(outbuff,"zebra seems not to have been started, "
                    "stopping anyway");
   zebra_stop(zs);
   zs=0;
   return 0; /* ok */
 }

int cmd_zebra_open( char *args[], char *outbuff)
{
  if (!zs)
    strcat(outbuff,"zebra seems not to have been started, "
                   "trying anyway");
  zh=zebra_open(zs);
  return 0; /* ok */
}

int cmd_zebra_close( char *args[], char *outbuff)
{
  if (!zh)
    strcat(outbuff,"Seems like you have not called zebra_open,"
                   "trying anyway");
  zebra_close(zh);
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
   { "zebra_start", "[configfile]", 
        "starts the zebra service", cmd_zebra_start },
   { "zebra_stop", "", 
        "stops the zebra service", cmd_zebra_stop },
   { "zebra_open", "", 
        "starts a zebra session", cmd_zebra_open },
   { "zebra_close", "", 
        "closes a zebra session", cmd_zebra_close },
   { "echo", "string", "ouputs the string", cmd_echo },
   { "quit", "", "exits the program", cmd_quit },
   { "help", "", 0, cmd_help },
   
   {0,0,0,0} /* end marker */
 };
 
 int onecommand( char *line, char *outbuff)
 {
   int i;
   char *argv[MAX_NO_ARGS];
   int n;
   char argbuf[MAX_ARG_LEN];
   strncpy(argbuf,line, MAX_ARG_LEN-1);
   argbuf[MAX_ARG_LEN-1]='\0'; /* just to be sure */
   n=split_args(argbuf, argv);
   if (0==n)
     return 0; /* no command on line, too bad */
   for (i=0;cmds[i].cmd;i++)
     if (0==strcmp(cmds[i].cmd, argv[0])) 
     {
       if (n>1)
         argv[0]= line + (argv[1]-argbuf); /* rest of the line */
       else
         argv[0]=""; 
       return ((cmds[i].testfunc)(argv,outbuff));
     }
   sprintf (outbuff, "Unknown command '%s'. Try help",argv[0] );
   return -1; 
 }
 
 int cmd_help( char *args[], char *outbuff)
 { 
   int i;
   char tmp[MAX_ARG_LEN];
   for (i=0;cmds[i].cmd;i++)
     if (cmds[i].explanation)
     {
       sprintf(tmp, "%s %s %s\n",
         cmds[i].cmd, cmds[i].args, cmds[i].explanation);
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
 
int main (int argc, char ** argv)
{
  shell();
  return 0;
} /* main */

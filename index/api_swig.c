#include <idzebra/api_swig.h>
#include <stdarg.h>

#define DEFAULT_APPROX_LIMIT 2000000000

/* == Internal structures ================================================== */

struct idzebra_swig_service {
  ZebraService zs;
  Res res;
};

struct idzebra_swig_session {
  ZebraHandle zh;
  Res res;
};

/* == API errors, debug ==================================================== */
static Res api_error = 0;
const char* api_error_context = 0;
void api_add_error(const char *fmt, ...); 
void api_clear_error(void); 
void free_array(const char **args);


/* == API init, destroy  =================================================== */
void idzebra_api_init(void) 
{
  yaz_log_init_prefix("API_SWIG");
  yaz_log(YLOG_LOG, "IDZebra API initialized");
}

/* == Service start/stop =================================================== */

IDZebraService idzebra_start (RES_LIST)
{
  IDZebraService srv = xmalloc(sizeof(*srv));

  ARGS_INIT;
  API_SET_CONTEXT;

  ARGS_PARSE ("configName", 
	      "chdir", 
	      "modulePath", 
	      "profilePath",
	      "register",
	      "shadow",
	      "isam",
	      "keyTmpDir",
	      "setTmpDir",
	      "lockDir");
  
  srv->res = func_res;
  srv->zs = zebra_start_res(res_get(func_res,"configName"), NULL, srv->res);

  func_res = 0; 
  ARGS_DONE;

  return (srv);
}

IDZEBRA_RES idzebra_stop(IDZebraService srv) 
{
  ZEBRA_RES rv = zebra_stop(srv->zs);
  res_close (srv->res);
  xfree (srv);
  return (rv);
}

/* == Session open/close =================================================== */
IDZebraSession idzebra_open (IDZebraService srv, RES_LIST)
{
  IDZebraSession sess = xmalloc(sizeof(*sess));

  ARGS_INIT;
  API_SET_CONTEXT;

  ARGS_PARSE  ("tempfiles", 
	       "openRW", 
	       "memmax",
	       "encoding",
	       "estimatehits",
	       "staticrank");

  sess->res = func_res;
  sess->zh = zebra_open(srv->zs, sess->res);

  /* Function resources are kept for session (zh->res->over_res); */
  func_res = 0; 
  ARGS_DONE;

  return (sess);
}

IDZEBRA_RES idzebra_close(IDZebraSession sess) 
{
  ZEBRA_RES rv = zebra_close (sess->zh);
  res_close (sess->res);
  xfree (sess);
  return (rv);
}
/* == Sample function == =================================================== */
IDZEBRA_RES idzebra_samplefunc(IDZebraSession sess, RES_LIST)
{
  ARGS_INIT;
  API_SET_CONTEXT;
  ARGS_PARSE("strucc");
  ARGS_PROCESS(ARG_MODE_OPTIONAL,"encoding");
  ARGS_APPLY;

  res_dump (sess->res,0);

  //  ARGS_REVOKE;
  {                                                              
  const char **used;                                             
  res_remove_over(temp_res);                                     
  used = (const char **) res_get_array(local, "_used");                          args_use(sess, sess->res, 0, ARG_MODE_FORCE, used);            
  free_array(used);                                              
  }                                                              
  ARGS_DONE;
  return (ZEBRA_OK);
} 


/* 
-------------------------------------------------------------------------------
 Utility functions for argument handling
-------------------------------------------------------------------------------
*/
void arg_parse_res (Res r, 
		    const char **valid_args, 
		    Res skip,
		    char *name, char *value)
{
  if ((name) && (value)) {
      int i = 0;
      int gotit = 0;
      while (valid_args[i]) {
	if (!strcmp(name, valid_args[i])) {
	  res_set(r, name, value);
	  gotit = 1;
	  break;
	}
	i++;
      }
      if (!gotit)
	yaz_log (YLOG_DEBUG, "skip: %s=%s",name, value);
	res_add (skip, name, value);
  }
}

void args_parse_res (Res r, 
		     const char **valid_args, 
		     Res skip,
		     char **args)
{
  char *argname;
  char *argvalue;
  int i = 0;
  if (args) {
    while (args[i] && args[i+1]) {
      argname = args[i++];
      argvalue = args[i++];
      arg_parse_res (r, valid_args, skip, argname, argvalue);
    }
  }
}

/* == special resource handlers ============================================ */

void idzebra_res_encoding (ZebraHandle zh, const char *value) 
{
  if (value)
    zebra_octet_term_encoding(zh, value);
  else
    zebra_octet_term_encoding(zh, "ISO-8859-1");
  
}

void idzebra_res_estimatehits (ZebraHandle zh, const char *value) 
{
  int val = 0;
  if (value)
    if (! (sscanf(value, "%d", &val) == 1)) 
      api_add_error( "Expected integer value for 'estimatehits'");

  zebra_set_approx_limit(zh, val);
}

/*
void idzebra_res_staticrank (ZebraHandle zh, const char *value) 
{
  int val = 0;
  if (value)
    if (! (sscanf(value, "%d", &val) == 1)) 
      api_add_error( "Expected integer value for 'estimatehits'");
  
  sess->zh->m_staticrank = val;
}
*/

/* == applying and revoking call-scope resources =========================== */

void arg_use (ZebraHandle zh,
	      Res r,
	      Res rr,
	      int mode,
	      const char *name)
{
  if (name) {
    const char *value = res_get(r, name);
    int gotit = 0;

    /* in FORCE mode resource is used with default value also */
    if ((value) || (mode & ARG_MODE_FORCE)) {

      /* encoding */
      if (!strcmp(name,"encoding")) {
	idzebra_res_encoding(zh, value);
	gotit = 1;
      }

      /* estimatehits */
      else if (!strcmp(name,"estimatehits")) {
	idzebra_res_estimatehits(zh, value);
	gotit = 1;
      }

      /* staticrank */
      /*
      else if (!strcmp(name,"staticrank")) {
	idzebra_res_staticrank(zh, value);
	gotit = 1;
      }
      */
      
      /* collects provided arguments in order to revoke them 
	 at the end of the function */
      if (gotit) {
	if (! (mode & ARG_MODE_FORCE)) res_add(r, "_used", name);
	if ( (rr) && (name) && (value) ) res_add(rr, name, value);
      }
  } else {
      /* value missing */
      if (mode & ARG_MODE_MANDATORY)
	api_add_error( "Missing mandatory argument '%s'", name);
    }
  }
}

void args_use (IDZebraSession sess,
	       Res r,
	       Res rr,
	       int mode,
	       const char **args)
{
  int i = 0;
  if (args) {
    while (args[i]) {
      arg_use (sess->zh, r, rr, mode, args[i++]);
    }
  }
}

/* == API errors =========================================================== */

void api_add_error(const char *fmt, ...) 
{

  va_list ap;
  char buf[4096];  
  const char *context;
  va_start(ap, fmt);

#ifdef WIN32
  _vsnprintf(buf, sizeof(buf)-1, fmt, ap);
#else
/* !WIN32 */
#if HAVE_VSNPRINTF
  vsnprintf(buf, sizeof(buf), fmt, ap);
#else
  vsprintf(buf, fmt, ap);
#endif
#endif

  va_end (ap);

  if (! api_error)
    api_error = res_open(0,0);

  if (api_error_context)
    context = api_error_context;
  else
    context = "<unknown>";
      
  res_add(api_error, context, buf);
}

char **api_errors(void)
{
  static char **res = 0;

  res = res_2_array(api_error);
  if (!res) {
    res = xmalloc(sizeof(char *));
    res[0] = 0 ;
  }
  return (&res[0]);

}


int api_check_error(void)
{
  if (api_error)
    return (1);
  else
    return (0);
}

void api_clear_error(void) 
{
  if (api_error)
    res_close(api_error);
  api_error = 0;
}

void free_array(const char **args)
{
  int i = 0;
  if (args) {
    while (args[i]) {
      xfree((char *) args[i]);
      i++;
    }
    xfree (args);
  }
}
/*
 * Local variables:
 * c-basic-offset: 4
 * indent-tabs-mode: nil
 * End:
 * vim: shiftwidth=4 tabstop=8 expandtab
 */


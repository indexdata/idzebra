#ifndef API_SWIG_H
#define API_SWIG_H

#include <yaz/xmalloc.h>
#include <idzebra/res.h>
#include <idzebra/api.h>

typedef short IDZEBRA_RES; 
#define RES_LIST char** res_args                                 

typedef struct idzebra_swig_service *IDZebraService;

typedef struct idzebra_swig_session *IDZebraSession;

/* 
-------------------------------------------------------------------------------
 API Calls
-------------------------------------------------------------------------------
*/
void idzebra_api_init(void);

char **api_errors(void);

int api_check_error(void);

void api_clear_error(void);


IDZebraService idzebra_start (RES_LIST);

IDZEBRA_RES idzebra_stop(IDZebraService srv);


IDZebraSession idzebra_open (IDZebraService srv, RES_LIST);

IDZEBRA_RES idzebra_close(IDZebraSession sess);

IDZEBRA_RES idzebra_samplefunc(IDZebraSession sess, RES_LIST);


/* 
-------------------------------------------------------------------------------
 Utility functions for argument handling
-------------------------------------------------------------------------------
*/

#if __STDC_VERSION__ < 199901L
# if __GNUC__ >= 2
#  define __func__ __FUNCTION__
# else
#  define __func__ "<unknown>"
# endif
#endif

#define API_SET_CONTEXT api_error_context = __func__

void args_parse_res (Res r, 
		     const char **valid_args, 
	 	     Res skip,
		     char **args);

void args_use (IDZebraSession sess,
	       Res r,
	       Res rr,
	       int mandatory,
	       const char **args);

#define ARG_MODE_OPTIONAL 0
#define ARG_MODE_MANDATORY 1
#define ARG_MODE_FORCE 2

#define RES_OPEN(var,def,over)                                   \
  var = res_open(def, over);                                     \
  res_set(var,"__context__", __func__ );                         \

#define ARGS_INIT                                                \
  Res local = 0;                                                 \
  Res func_res = 0;                                              \
  Res temp_res = 0;                                              \
  
#define ARGS_PARSE(...)                                          \
  {                                                              \
  const char *vargs[] = { __VA_ARGS__ , 0 };                     \
  RES_OPEN(func_res, 0, 0);                                      \
  RES_OPEN(local, 0, 0);                                         \
  args_parse_res(func_res, vargs, local, res_args);              \
  }                                                              \

#define ARGS_APPLY                                               \
  temp_res = res_add_over(sess->res, func_res);                  \

#define ARGS_PROCESS(mode, ...)                                  \
  {                                                              \
  const char *vargs[] = { __VA_ARGS__ , 0 };                     \
  args_use(sess, local, func_res, mode, vargs);                  \
  }                                                              \

#define ARGS_REVOKE                                              \
  {                                                              \
  const char **used;                                             \
  res_remove_over(temp_res);                                     \
  used = res_get_array(local, "_used");                          \
  args_use(sess, sess->res, 0, ARG_MODE_FORCE, used);              \
  free_array(used);                                              \
  }                                                              \

#define ARGS_DONE                                                \
  if (func_res) res_close(func_res);                             \
  if (temp_res) res_close_over(temp_res);                        \
  if (local) res_close(local);                                   \

#endif /* API_SWIG_H */

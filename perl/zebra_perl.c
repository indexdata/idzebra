//#include "zebraapi.h"
#include "zebra_perl.h"
#include <data1.h>
#include <yaz/log.h>
#include "rg.h"

NMEM handles;

void init (void) {
  nmem_init ();
  yaz_log_init_prefix ("ZebraPerl");
  yaz_log (LOG_LOG, "Zebra::API initialized");
}

void DESTROY (void) {
  nmem_exit ();
  yaz_log (LOG_LOG, "Zebra::API destroyed");
}

/* Logging facilities from yaz */
void logLevel (int level) {
  yaz_log_init_level(level);
}
 
void logFile (const char *fname) {
  yaz_log_init_file(fname);
}

void logMsg (int level, const char *message) {
  logf(level, "%s", message);
}



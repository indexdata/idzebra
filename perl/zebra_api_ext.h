#ifndef ZEBRA_API_EXT_H
#define ZEBRA_API_EXT_H

#include "zebraapi.h"

void api_records_retrieve (ZebraHandle zh, ODR stream,
			   const char *setname, Z_RecordComposition *comp,
			   oid_value input_format, int num_recs,
			   ZebraRetrievalRecord *recs);
typedef struct {
  char *buf;           /* record buffer (void pointer really) */
  int len;             /* length */
} RetrievalRecordBuf;


#endif

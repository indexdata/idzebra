#ifndef ZEBRA_PERL_H
#define ZEBRA_PERL_H

#include <idzebra/api.h>

typedef struct {
    char  *groupName;
    char  *databaseName;
    char  *path;
    char  *recordId;
    char  *recordType;
    int   flagStoreData;
    int   flagStoreKeys;
    int   flagRw;
    int   fileVerboseLimit;
    int   databaseNamePath;
    int   explainDatabase;
    int   followLinks;
} recordGroup;

typedef struct {
  int noOfRecords;
  ZebraRetrievalRecord *records;
} RetrievalObj;

typedef struct {
  char *buf;           /* record buffer (void pointer really) */
  int len;             /* length */
} RetrievalRecordBuf;


typedef struct {
  int  errCode;        /* non-zero if error when fetching this */
  char *errString;     /* error string */
  int  position;       /* position of record in result set (1,2,..) */
  char *base; 
  SYSNO sysno;
  int  score;
  char *format;    /* record syntax */
  RetrievalRecordBuf *buf;
} RetrievalRecord;

/* Scan Term Descriptor */
typedef struct {
    int occurrences;     /* scan term occurrences */
    char *term;          /* scan term string */
} scanEntry;

typedef struct {
  int num_entries;
  int position;
  int is_partial;
  scanEntry *entries;
} ScanObj;

#endif



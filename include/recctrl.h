/*
 * Copyright (C) 1994-1996, Index Data I/S 
 * All rights reserved.
 * Sebastian Hammer, Adam Dickmeiss
 *
 * $Log: recctrl.h,v $
 * Revision 1.16  1996-10-11 10:56:25  adam
 * New module recctrl. Used to manage records (extract/retrieval).
 * All record types are accessed by means of definitions in recctrl.h.
 *
 * Revision 1.15  1996/06/06 12:08:16  quinn
 * Added showRecord Group entry
 *
 * Revision 1.14  1996/05/09  07:28:49  quinn
 * Work towards phrases and multiple registers
 *
 * Revision 1.13  1996/05/01  13:44:05  adam
 * Added seek function to the recExtractCtrl and recRetrieveCtrl control
 * structures. Added end-of-file indicator function and start offset to
 * recExtractCtrl.
 *
 * Revision 1.12  1996/01/17  15:01:25  adam
 * Prototype changed for reader functions in extract/retrieve. File
 *  is identified by 'void *' instead of 'int'.
 *
 * Revision 1.11  1995/12/04  14:20:54  adam
 * Extra arg to recType_byName.
 *
 * Revision 1.10  1995/10/16  14:03:06  quinn
 * Changes to support element set names and espec1
 *
 * Revision 1.9  1995/10/06  14:37:53  adam
 * New result set method: r_score.
 * Local no (sysno) and score is transferred to retrieveCtrl.
 *
 * Revision 1.8  1995/10/02  15:43:35  adam
 * Extract uses file descriptors instead of FILE pointers.
 *
 * Revision 1.7  1995/10/02  15:18:09  adam
 * Minor changes.
 *
 * Revision 1.6  1995/10/02  15:05:26  quinn
 * Added a few fields.
 *
 * Revision 1.5  1995/10/02  14:55:52  quinn
 * *** empty log message ***
 *
 * Revision 1.4  1995/09/27  16:17:29  adam
 * More work on retrieve.
 *
 * Revision 1.3  1995/09/27  12:21:25  adam
 * New function: recType_byName.
 *
 * Revision 1.2  1995/09/15  14:45:03  adam
 * Retrieve control.
 *
 * Revision 1.1  1995/09/14  07:48:13  adam
 * Record control management.
 *
 */

#ifndef RECCTRL_H
#define RECCTRL_H

#include <proto.h>
#include <oid.h>
#include <odr.h>

/* single word entity */
typedef struct {
    int  attrSet;
    int  attrUse;
    enum {
	Word_String,
	Word_Phrase,
        Word_Numeric
    } which;
    union {
        char *string;
        int  numeric;
    } u;
    int seqno;
} RecWord;

/* Extract record control */
struct recExtractCtrl {
    void      *fh;                    /* File handle and read function     */
    int       (*readf)(void *fh, char *buf, size_t count);
    off_t     (*seekf)(void *fh, off_t offset);  /* seek function          */
    void      (*endf)(void *fh, off_t offset);   /* end of record position */
    off_t     offset;                            /* start offset           */
    char      *subType;
    void      (*init)(RecWord *p);
    void      (*add)(const RecWord *p);
    char **(*map_chrs_input)(char **from, int len);
    int       flagShowRecords;
};

/* Retrieve record control */
struct recRetrieveCtrl {
    /* Input parameters ... */
    ODR       odr;                    /* ODR used to create response       */
    void     *fh;                     /* File descriptor and read function */
    int       (*readf)(void *fh, char *buf, size_t count);
    off_t     (*seekf)(void *fh, off_t offset);
    oid_value input_format;           /* Preferred record syntax           */
    Z_RecordComposition *comp;        /* formatting instructions           */
    int       localno;                /* local id of record                */
    int       score;                  /* score 0-1000 or -1 if none        */
    char      *subType;
    
    /* response */
    oid_value  output_format;
    void       *rec_buf;
    size_t     rec_len;
    int        diagnostic;
    char *message;
};

typedef struct recType
{
    char *name;                       /* Name of record type */
    void (*init)(void);               /* Init function - called once       */
    int  (*extract)(struct recExtractCtrl *ctrl);     /* Extract proc      */
    int  (*retrieve)(struct recRetrieveCtrl *ctrl);   /* Retrieve proc     */
} *RecType;

RecType recType_byName (const char *name, char *subType);

#endif

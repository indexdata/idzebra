/*
 * Copyright (C) 1994-1995, Index Data I/S 
 * All rights reserved.
 * Sebastian Hammer, Adam Dickmeiss
 *
 * $Log: recctrl.h,v $
 * Revision 1.4  1995-09-27 16:17:29  adam
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

#include <oid.h>
#include <odr.h>

/* single word entity */
typedef struct {
    int  attrSet;
    int  attrUse;
    enum {
	Word_String,
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
    /* input parameters ... */
    FILE *inf;                         /* Extract from this file */
    char *subType;                     /* Sub type - may be NULL */
    void (*init)(RecWord *p);          /* Init of word spec */
    void (*add)(const RecWord *p);     /* Addition of a single word */
};

/* Retrieve record control */
struct recRetrieveCtrl {
    /* Input parameters ... */
    ODR        odr;                    /* ODR used to create response */
    int        fd;                     /* File descriptor and read function */
    int       (*readf)(int fd, char *buf, size_t count);
    oid_value  input_format;           /* Preferred record syntax */
    
    /* output parameters ... */
    oid_value  output_format;          /* Record syntax of returned record */
    void       *rec_buf;               /* Record buffer */
    size_t     rec_len;                /* Length of record */
};

typedef struct recType
{
    char *name;                        /* Name of record type */
    void (*init)(void);                /* Init function - called once */
    int  (*extract)(struct recExtractCtrl *ctrl);     /* Extract proc */
    int  (*retrieve)(struct recRetrieveCtrl *ctrl);   /* Retrieve proc */
} *RecType;

RecType recType_byName (const char *name);

#endif

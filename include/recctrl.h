/*
 * Copyright (C) 1994-1995, Index Data I/S 
 * All rights reserved.
 * Sebastian Hammer, Adam Dickmeiss
 *
 * $Log: recctrl.h,v $
 * Revision 1.3  1995-09-27 12:21:25  adam
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

struct recExtractCtrl {
    FILE *inf;
    char *subType;
    void (*init)(RecWord *p);
    void (*add)(const RecWord *p);
};

struct recRetrieveCtrl {
    ODR        odr;
    int        fd;
    int       (*readf)(int fd, char *buf, size_t count);
    oid_value  input_format;
    
    /* response */
    oid_value  output_format;
    void       *rec_buf;
    size_t     rec_len;
};

typedef struct recType
{
    char *name;
    void (*init)(void);
    int  (*extract)(struct recExtractCtrl *ctrl);
    int  (*retrieve)(struct recRetrieveCtrl *ctrl);
} *RecType;

RecType recType_byName (const char *name);

#endif

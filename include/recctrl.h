/*
 * Copyright (C) 1994-1995, Index Data I/S 
 * All rights reserved.
 * Sebastian Hammer, Adam Dickmeiss
 *
 * $Log: recctrl.h,v $
 * Revision 1.1  1995-09-14 07:48:13  adam
 * Record control management.
 *
 */

#ifndef RECCTRL_H
#define RECCTRL_H

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

typedef struct recType
{
    char *name;
    void (*init)(void);
    int  (*extract)(struct recExtractCtrl *ctrl);
} *RecType;

#endif

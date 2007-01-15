/* $Id: isam.h,v 1.5 2007-01-15 20:08:24 adam Exp $
   Copyright (C) 1995-2007
   Index Data ApS

This file is part of the Zebra server.

Zebra is free software; you can redistribute it and/or modify it under
the terms of the GNU General Public License as published by the Free
Software Foundation; either version 2, or (at your option) any later
version.

Zebra is distributed in the hope that it will be useful, but WITHOUT ANY
WARRANTY; without even the implied warranty of MERCHANTABILITY or
FITNESS FOR A PARTICULAR PURPOSE.  See the GNU General Public License
for more details.

You should have received a copy of the GNU General Public License
along with this program; if not, write to the Free Software
Foundation, Inc., 51 Franklin St, Fifth Floor, Boston, MA  02110-1301  USA

*/

/**
isam.h - a generalized interface to the isam systems

The isam system consists of a number of isam lists. Physically it is 
stored in a file, or a group of related files. It is typically used 
for storing all the occurrences of a given word, storing the document 
number and position for each occurrence. 

An isam list is indentified by an isam_position. This is a number (zint).
It can be seen as a mapping from an isam_position to an ordered list of isam_
entries.

An isam list consists of one or more isam entries. We do not know the 
structure of those entries, but we know the (maximum) size of such, and
that they can be memcpy'ed around.

The entries can be seen to consist of a key and a value, although we 
have no idea of their internal structure. We know that we have a compare
function that can look at a part (or whole) of the isam entry (the 'key'). 
The part not looked at (if any) will count as 'value' or 'payload'. 

The entries are stored in increasing order (as defined by the compare
function), and no duplicates are allowed.

There is an effective mass-insert routine that takes a stream of values, 
each accompanied by an insert/delete flag.

For reading we have cursors, that can read through an isam list in order. 
They have a fast-forward function to skip values we are not interested in.
 
*/


#ifndef ISAM_H
#define ISAM_H

#include <idzebra/bfile.h>

#ifdef __cplusplus
extern "C" {
#endif


/**
 * key_control contains all there is to know about the keys (entries) stored
 * in an isam, (and therefore operated by the rsets). Other than this info,
 * all we assume is that all keys are the same size, and they can be
 * memcpy'd around. 
 */
struct key_control {
    /** (max) size of a key */
    int key_size;

    /** Default for what level we operate on (book/chapter/verse). 
     * for typical zebra, this is always 2 (sysno/seqno). Not used in 
     * isam context, but the rsets make use of this. */
    int scope; 

    /** Compare function, returning -1,0,1, if p1 is less/equal/greater 
     * than p2 */
    int (*cmp) (const void *p1, const void *p2);

    /** Debug function to write a key in the log, with a message */
    void (*key_logdump_txt) (int logmask, const void *p, const char *txt);

    /** Return the sequence number of a key, to see if we are on the same 
     * record. FIXME - this makes less sense with higher-scope keys. */
    zint (*getseq)(const void *p);

    /** Codec to pack key values into a disk page (delta-compression etc) */
    ISAM_CODEC *codec;
};

typedef struct key_control KEY_CONTROL;

const KEY_CONTROL *default_key_control();
  /* FIXME - in zrpn.c, time being. Needs to be moved out */


/** isam_data_stream is a callback function for the mass-insert (merge) 
 * it provides another item to insert/delete, in proper order */
struct isam_data_stream {
    int (*read_item)(void *clientData, char **dst, int *insertMode);
    void *clientData;
};

typedef struct isam_data_stram ISAM_DATA_STREAM;


/** ISAM_POS is a number the ISAM translates from */
typedef zint ISAM_POS;

/** ISAM is a translation from POS to a set of values */
typedef struct ISAM_s *ISAM;

/** ISAM_CUR is a pointer into an ISAM */
typedef struct ISAM_CUR_s *ISAM_CUR;

/** isam_control is the interface to the operations an ISAM supports */
struct isam_conrol {
    /** text description of the type, for debugging */
    char *desc;
    /** default filename, if none given to isam_open */
    const char *def_filename; 

    /* there is an isam_open function, but it is not part of this */
    /* dynamic table, as it is what provides this table */

    /** close the isam system */
    void (*f_close)(ISAM i);

    /** Insert an entry into the isam identified by pos. If pos==0, 
     * create a new isam list */
    ISAM_POS (*f_put)(ISAM is, ISAM_POS pos, const void *buf);

    /** Locate and delete an entry from an isam list. If not there
     * do nothing, and return 0*/
    int (*f_del)(ISAM is, ISAM_POS pos, const void *buf);

    /** Find an entry in the isam list. return 0 if not found. buf must 
     * contain enough to identify the item, and will be overwritten by it */
    int (*f_get)(ISAM is, ISAM_POS pos, void *buf );

    /** Mass-insert data from incoming stream into the isam */
    ISAM_POS (*f_merge)(ISAM is, ISAM_POS pos, ISAM_DATA_STREAM *data); 

    /** Open a cursor to the isam list identified by pos */
    ISAM_CUR (*f_cur_open)(ISAM is, ISAM_POS pos); 

    /** Read an item at the cursor (and forward to next). return 0 at eof */
    int (*f_read)(ISAM_CUR cur, void *buf);

    /** Forward until item >= untilbuf, and read that item. Skips effectively*/
    int (*f_forward)(ISAM_CUR cur, void *buf, const void *untilbuf);

    /** Get (an estimate of) the current position and total size of the entry*/
    void (*f_pos)(ISAM_CUR cur, double *current, double *total);

    /** Close a cursor */
    void (*f_cur_close)(ISAM_CUR cur);

    /** Delete the isam list from the isam system.*/
    int (*f_unlink)(ISAM is, ISAM_POS pos);
    
};

/** ISAM_s is the generic isam structure */
struct ISAM_s {
    const struct isam_control *ictrl;  /* the functions */
    const KEY_CONTROL *kctrl; /* all about the keys stored in the isam */
    BFiles bfs;  /* The underlying block file system */
    void *priv; /* various types of ISAMs hand their private parts here */
};

/** ISAM_CUR is a cursor to an ISAM, used for reading the next value, etc. */
struct ISAM_CUR {
    ISAM is;
    void *priv;
};



/** Open the isam system */
ISAM isam_open (BFiles bfs, 
                const char *isamtype, /* usually "b" */
                const char *filename,  /* optional, use default from control ?? */
                int flags, /* FIXME - define read/write, and some special ones */
                const KEY_CONTROL *key_control);


/** Shortcut defines to access the functions through the key_control block */

#define isam_close(is) (*(is)->ictrl->f_close)(is)

#define isam_puf(is,pos,buf) (*(is)->ictrl->f_put)((is),(pos)(buf))

#define isam_del(is,pos,buf) (*(is)->ictrl->f_del)((is),(pos)(buf))

#define isam_get(is,pos,buf) (*(is)->ictrl->f_get)((is),(pos)(buf))

#define isam_merge(is,pos,data) (*(is)->ictrl->f_merge)((is),(pos)(data))

#define isam_cur_open(is,pos) (*(is)->ictrl->f_cur_open)((is),(pos))

#define isam_read(cur,buf) (*(is)->ictrl->f_read)((cur),(buf))

#define isam_forward(cur,buf,untilbuf) (*(is)->ictrl->f_forward)((cur),(buf)(untilbuf))

#define isam_pos(cur,current,total) (*(is)->ictrl->f_pos)((cur),(current),(total))

#define isam_cur_close(cur) (*(is)->ictrl->f_cur_close)(cur)

#define isam_unlink(is,pos) (*(is)->ictrl->f_unlink)((is),(pos))


#ifdef __cplusplus
}
#endif

#endif  /* ISAM_H */
/*
 * Local variables:
 * c-basic-offset: 4
 * indent-tabs-mode: nil
 * End:
 * vim: shiftwidth=4 tabstop=8 expandtab
 */


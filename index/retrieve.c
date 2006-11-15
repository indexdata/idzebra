/* $Id: retrieve.c,v 1.50 2006-11-15 14:09:43 marc Exp $
   Copyright (C) 1995-2006
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

#include <stdio.h>
#include <assert.h>

#include <fcntl.h>
#ifdef WIN32
#include <io.h>
#include <process.h>
#endif
#if HAVE_UNISTD_H
#include <unistd.h>
#endif

#include "index.h"
#include <yaz/diagbib1.h>
#include <direntz.h>




static int zebra_create_record_stream(ZebraHandle zh, 
                               Record *rec,
                               struct ZebraRecStream *stream){

    RecordAttr *recordAttr = rec_init_attr(zh->reg->zei, *rec);

    if ((*rec)->size[recInfo_storeData] > 0){ 
        zebra_create_stream_mem(stream, (*rec)->info[recInfo_storeData],
                                (*rec)->size[recInfo_storeData]);
    }
    else {
        char full_rep[1024];
        int fd;
            
        if (zh->path_reg && !yaz_is_abspath((*rec)->info[recInfo_filename])){
            strcpy(full_rep, zh->path_reg);
            strcat(full_rep, "/");
            strcat(full_rep, (*rec)->info[recInfo_filename]);
        }
        else
            strcpy(full_rep, (*rec)->info[recInfo_filename]);
            
        if ((fd = open (full_rep, O_BINARY|O_RDONLY)) == -1){
            yaz_log (YLOG_WARN|YLOG_ERRNO, "Retrieve fail; missing file: %s",
                     full_rep);
            rec_free(rec);
            return 14;
        }
        zebra_create_stream_fd(stream, fd, recordAttr->recordOffset);
    }
    return 0;
}
    


static void parse_zebra_elem(const char *elem,
                             const char **index, size_t *index_len,
                             const char **type, size_t *type_len)
{
    *type = 0;
    *type_len = 0;

    *index = 0;
    *index_len = 0;

    if (elem && *elem)
    {
        const char *cp = strchr(elem, ':');

        if (!cp) /* no colon */
        {
            *index = elem;
            *index_len = strlen(elem);
        }
        else if (cp[1] == '\0') /* 'index:' */
        {
            *index = elem;
            *index_len = cp - elem;
        }
        else
        {
            *index = elem;
            *index_len = cp - elem;
            *type = cp+1;
            *type_len = strlen(cp+1);
        }
    }
}


int zebra_special_index_fetch(ZebraHandle zh, SYSNO sysno, ODR odr,
                          Record rec,
                          const char *elemsetname,
                          oid_value input_format,
                          oid_value *output_format,
                          char **rec_bufp, int *rec_lenp)
{
    const char *retrieval_index;
    size_t retrieval_index_len; 
    const char *retrieval_type;
    size_t retrieval_type_len;
   
    /* int return_code = YAZ_BIB1_SYSTEM_ERROR_IN_PRESENTING_RECORDS; */

    WRBUF wrbuf = wrbuf_alloc();
    zebra_rec_keys_t keys;
    
    /* only accept XML and SUTRS requests */
    if (input_format != VAL_TEXT_XML
        && input_format != VAL_SUTRS){
        yaz_log(YLOG_WARN, "unsupported format for element set zebra::%s", 
                elemsetname);
        *output_format = VAL_NONE;
        return YAZ_BIB1_SYSTEM_ERROR_IN_PRESENTING_RECORDS;
    }

    keys = zebra_rec_keys_open();
    zebra_rec_keys_set_buf(keys, rec->info[recInfo_delKeys],
                           rec->size[recInfo_delKeys], 0);

    parse_zebra_elem(elemsetname,
                     &retrieval_index, &retrieval_index_len,
                     &retrieval_type,  &retrieval_type_len);

    if (zebra_rec_keys_rewind(keys)){
        size_t slen;
        const char *str;
        struct it_key key_in;

        if (input_format == VAL_TEXT_XML){
            *output_format = VAL_TEXT_XML;
            /*wrbuf_printf(wrbuf, 
              "<?xml version=\"1.0\" encoding=\"UTF-8\"?>\n");*/
            
            wrbuf_printf(wrbuf, 
                         "<record xmlns="
                         "\"http://www.indexdata.com/zebra/\""
                         " sysno=\"" ZINT_FORMAT "\""
                         " set=\"zebra::index::%s\">\n",
                         sysno, elemsetname);
        }
        else if (input_format == VAL_SUTRS)
            *output_format = VAL_SUTRS;


        while(zebra_rec_keys_read(keys, &str, &slen, &key_in)){
            int i;
            int ord = CAST_ZINT_TO_INT(key_in.mem[0]);
            int index_type;
            const char *db = 0;
            const char *string_index = 0;
            size_t string_index_len;
            char dst_buf[IT_MAX_WORD];
            
            zebraExplain_lookup_ord(zh->reg->zei, ord, &index_type, &db,
                                    &string_index);
            string_index_len = strlen(string_index);
            if (retrieval_index == 0 
                || (string_index_len == retrieval_index_len 
                    && !memcmp(string_index, retrieval_index,
                               string_index_len))){
                
                if (retrieval_type == 0 
                    || (retrieval_type_len == 1 
                        && retrieval_type[0] == index_type)){
                    
                    if (input_format == VAL_TEXT_XML){
                        wrbuf_printf(wrbuf, "  <index name=\"%s\"", 
                                     string_index);
                        
                        wrbuf_printf(wrbuf, " type=\"%c\"", index_type);
                        
                        wrbuf_printf(wrbuf, " seq=\"" ZINT_FORMAT "\">", 
                                     key_in.mem[key_in.len -1]);
                        
                        zebra_term_untrans(zh, index_type, dst_buf, str);
                        wrbuf_xmlputs(wrbuf, dst_buf);
                        wrbuf_printf(wrbuf, "</index>\n");
                    }
                    else if (input_format == VAL_SUTRS){
                        wrbuf_printf(wrbuf, "%s ", string_index);
                    
                        wrbuf_printf(wrbuf, "%c", index_type);
                    
                        for (i = 1; i < key_in.len; i++)
                            wrbuf_printf(wrbuf, " " ZINT_FORMAT, 
                                         key_in.mem[i]);

                        zebra_term_untrans(zh, index_type, dst_buf, str);
                        wrbuf_printf(wrbuf, " %s", dst_buf);
                        
                        wrbuf_printf(wrbuf, "\n");
                    }
                }
            }
        }
        if (input_format == VAL_TEXT_XML)
            wrbuf_printf(wrbuf, "</record>\n");
     }
    
    *rec_lenp = wrbuf_len(wrbuf);
    *rec_bufp = odr_malloc(odr, *rec_lenp);
    memcpy(*rec_bufp, wrbuf_buf(wrbuf), *rec_lenp);
    wrbuf_free(wrbuf, 1);
    zebra_rec_keys_close(keys);
    return 0;
}


int zebra_special_fetch(ZebraHandle zh, SYSNO sysno, ODR odr,
                           const char *elemsetname,
                           oid_value input_format,
                           oid_value *output_format,
                           char **rec_bufp, int *rec_lenp)
{
    Record rec;

    /* only accept XML and SUTRS requests */
    if (input_format != VAL_TEXT_XML
        && input_format != VAL_SUTRS){
        yaz_log(YLOG_WARN, "unsupported format for element set zebra::%s", 
                elemsetname);
        return YAZ_BIB1_SYSTEM_ERROR_IN_PRESENTING_RECORDS;
    }

    /* processing zebra::meta::sysno elemset without fetching binary data */
    if (elemsetname  && 0 == strcmp(elemsetname, "meta::sysno"))
    {
	char rec_str[128];
        if (input_format == VAL_SUTRS){
            sprintf(rec_str, ZINT_FORMAT, sysno);
            *output_format = VAL_SUTRS;
        } 
        else if (input_format == VAL_TEXT_XML){
            sprintf(rec_str, "<record xmlns="
                             "\"http://www.indexdata.com/zebra/\""
                             " sysno=\"" ZINT_FORMAT "\""
                             " set=\"zebra::%s\">\n",
                             sysno, elemsetname);
            *output_format = VAL_TEXT_XML;
        }
	*rec_lenp = strlen(rec_str);
        if (*rec_lenp){
            *rec_bufp = odr_strdup(odr, rec_str);
            return 0;
        } else {
            return YAZ_BIB1_SYSTEM_ERROR_IN_PRESENTING_RECORDS;
        }
    }

    /* fetching binary record up for all other display elementsets */
    rec = rec_get(zh->reg->records, sysno);
    if (!rec){
        yaz_log(YLOG_DEBUG, "rec_get fail on sysno=" ZINT_FORMAT, sysno);
        return YAZ_BIB1_SYSTEM_ERROR_IN_PRESENTING_RECORDS;
    }

    /* processing special elementsetnames zebra::index:: */
    if (elemsetname && 0 == strncmp(elemsetname, "index::", 7)){
        
        int ret = zebra_special_index_fetch(zh, sysno, odr, rec,
                                            elemsetname + 7,
                                            input_format, output_format,
                                            rec_bufp, rec_lenp);
        
        rec_free(&rec);
        return ret;
    }

    /* processing special elementsetnames zebra::data */    
    if (elemsetname && 0 == strcmp(elemsetname, "data")){
        struct ZebraRecStream stream;
        RecordAttr *recordAttr = rec_init_attr(zh->reg->zei, rec); 
        zebra_create_record_stream(zh, &rec, &stream);
        *output_format = VAL_SUTRS;
        *rec_lenp = recordAttr->recordSize;
        *rec_bufp = (char *) odr_malloc(odr, *rec_lenp);
        stream.readf(&stream, *rec_bufp, *rec_lenp);
        stream.destroy(&stream);
        rec_free(&rec);
        return 0;
    }

    if (rec)
        rec_free(&rec);
    return YAZ_BIB1_SYSTEM_ERROR_IN_PRESENTING_RECORDS;
}

                          
int zebra_record_fetch(ZebraHandle zh, SYSNO sysno, int score,
                       zebra_snippets *hit_snippet, ODR odr,
                       oid_value input_format, Z_RecordComposition *comp,
                       oid_value *output_format,
                       char **rec_bufp, int *rec_lenp, char **basenamep,
                       char **addinfo)
{
    Record rec;
    char *fname, *file_type, *basename;
    const char *elemsetname;
    struct ZebraRecStream stream;
    RecordAttr *recordAttr;
    void *clientData;
    int return_code = 0;

    *basenamep = 0;
    *addinfo = 0;
    elemsetname = yaz_get_esn(comp);

    /*
    yaz_log(YLOG_LOG, "ELEMENTSET: '%s'", elemsetname);
    if (comp && comp->which)
        {
            yaz_log(YLOG_LOG, "%i %i", comp, comp->which);
        }
        If SRU, comp->which ==2 , and yaz_get_esn(comp) does not work correctly
        IF Z3055, comp->which ==1 , and everything is fine - except reord 
        encodning
    */

    /* processing zebra special elementset names of form 'zebra:: */
    /* SUGGESTION: do not check elemset nema here, buuuut ... */  
    if (elemsetname  && 0 == strncmp(elemsetname, "zebra::", 7))
        return  zebra_special_fetch(zh, sysno, odr,
                                    elemsetname + 7,
                                    input_format, output_format,
                                    rec_bufp, rec_lenp);


    /* processing all other element set names */
    rec = rec_get(zh->reg->records, sysno);
    if (!rec)
    {
        yaz_log(YLOG_DEBUG, "rec_get fail on sysno=" ZINT_FORMAT, sysno);
        *basenamep = 0;
        return YAZ_BIB1_SYSTEM_ERROR_IN_PRESENTING_RECORDS;
    }


    recordAttr = rec_init_attr(zh->reg->zei, rec);

    file_type = rec->info[recInfo_fileType];
    fname = rec->info[recInfo_filename];
    basename = rec->info[recInfo_databaseName];
    *basenamep = (char *) odr_malloc (odr, strlen(basename)+1);
    strcpy (*basenamep, basename);


    yaz_log (YLOG_DEBUG, "retrieve localno=" ZINT_FORMAT " score=%d",
             sysno, score);

    zebra_create_record_stream(zh, &rec, &stream);
    
    {
	/* snippets code */
	zebra_snippets *snippet;
	zebra_rec_keys_t reckeys = zebra_rec_keys_open();
        RecType rt;
        struct recRetrieveCtrl retrieveCtrl;

        retrieveCtrl.stream = &stream;
        retrieveCtrl.fname = fname;
        retrieveCtrl.localno = sysno;
        retrieveCtrl.staticrank = recordAttr->staticrank;
        retrieveCtrl.score = score;
        retrieveCtrl.recordSize = recordAttr->recordSize;
        retrieveCtrl.odr = odr;
        retrieveCtrl.input_format = retrieveCtrl.output_format = input_format;
        retrieveCtrl.comp = comp;
        retrieveCtrl.encoding = zh->record_encoding;
        retrieveCtrl.diagnostic = 0;
        retrieveCtrl.addinfo = 0;
        retrieveCtrl.dh = zh->reg->dh;
        retrieveCtrl.res = zh->res;
        retrieveCtrl.rec_buf = 0;
        retrieveCtrl.rec_len = -1;
        retrieveCtrl.hit_snippet = hit_snippet;
        retrieveCtrl.doc_snippet = zebra_snippets_create();

	zebra_rec_keys_set_buf(reckeys,
			       rec->info[recInfo_delKeys],
			       rec->size[recInfo_delKeys], 
			       0);
	zebra_rec_keys_to_snippets(zh, reckeys, retrieveCtrl.doc_snippet);
	zebra_rec_keys_close(reckeys);

#if 0
	/* for debugging purposes */
	yaz_log(YLOG_LOG, "DOC SNIPPET:");
	zebra_snippets_log(retrieveCtrl.doc_snippet, YLOG_LOG);
	yaz_log(YLOG_LOG, "HIT SNIPPET:");
	zebra_snippets_log(retrieveCtrl.hit_snippet, YLOG_LOG);
#endif
	snippet = zebra_snippets_window(retrieveCtrl.doc_snippet,
					retrieveCtrl.hit_snippet,
					10);
#if 0
	/* for debugging purposes */
	yaz_log(YLOG_LOG, "WINDOW SNIPPET:");
	zebra_snippets_log(snippet, YLOG_LOG);
#endif

        /* SUGGESTION: do not check elemset name here, buuuut ... 
           add another recType Struct with zebra internal stuff here,
           which overrides the Alvis/GRS-1/Safari filters .... 
        */
        if (!(rt = recType_byName(zh->reg->recTypes, zh->res,
                                  file_type, &clientData)))
        {
            return_code = YAZ_BIB1_SYSTEM_ERROR_IN_PRESENTING_RECORDS;
        }
        else
        {
        /* SUGGESTION: do not check elemset name here, buuuut ... 
           add another recType Struct with zebra internal stuff here,
           which overrides the Alvis/GRS-1/Safari filters .... 
           (*rt->retrieve) method to make the correct encoded, etc, retrieval,
           where all needed info already is found in the   &retrieveCtr
           parameter. This way, we do not need to re-code/dublicate a lot of 
           logic. 
        */
            (*rt->retrieve)(clientData, &retrieveCtrl);
            return_code = retrieveCtrl.diagnostic;

            *output_format = retrieveCtrl.output_format;
            *rec_bufp = (char *) retrieveCtrl.rec_buf;
            *rec_lenp = retrieveCtrl.rec_len;
            *addinfo = retrieveCtrl.addinfo;
        }
        /* another SUGGESTION: throw out all this snippet stuff in this 
           file, and do it correctly - either inside filters, or in another 
           new   (*rt->retrieve)(clientData, &retrieveCtrl); type 'snippet'
        */

	zebra_snippets_destroy(snippet);
        zebra_snippets_destroy(retrieveCtrl.doc_snippet);
     }

    stream.destroy(&stream);
    rec_free(&rec);

    return return_code;
}

/*
 * Local variables:
 * c-basic-offset: 4
 * indent-tabs-mode: nil
 * End:
 * vim: shiftwidth=4 tabstop=8 expandtab
 */


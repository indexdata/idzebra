/* $Id: csvread.c,v 1.4 2006-04-26 11:12:31 adam Exp $
   Copyright (C) 1995-2005
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
along with Zebra; see the file LICENSE.zebra.  If not, write to the
Free Software Foundation, 59 Temple Place - Suite 330, Boston, MA
02111-1307, USA.
*/



#include <yaz/log.h>
#include <yaz/nmem.h>
#include <yaz/yaz-util.h>

/* #include <d1_absyn.h> */
#include <idzebra/data1.h>
#include <idzebra/recgrs.h>

/* #include <assert.h> */
#include <ctype.h>

/*
struct csv_getc_info {
    char *buf;
    int buf_size;
    int size;
    int off;
    off_t moffset;
    void *fh;
    int (*readf)(void *, char *, size_t);
    WRBUF wrbuf;
};
*/

struct csv_t {
  NMEM nmem;
  int buf_size;
  char *buf;
  int name_size;
  int value_size;
  char *value;
  char field_char;
  char record_char;
  char string_char;
  char *root_element;
  int field_line;
  int lower_case;
  int max_nr_fields;
  int nr_fields;
  /* char *field_names; */
  char **field_name;
};


static ZEBRA_RES grs_config_csv(void *clientData, Res res, const char *args)
{
  int i;
  struct csv_t *csvp = (struct csv_t*) clientData;

  yaz_log (YLOG_LOG, "Called CSV filter grs_config_csv");
  yaz_log (YLOG_LOG, "'%s'", args);

  csvp->buf_size = 64;
  csvp->buf = nmem_malloc(csvp->nmem, csvp->buf_size);
  csvp->name_size = 256;
  csvp->value_size = 4096;
  csvp->value = nmem_malloc(csvp->nmem, csvp->value_size);

  csvp->field_char = '|';
  csvp->record_char = '\n';
  csvp->string_char = 0;    
  csvp->root_element = nmem_strdup(csvp->nmem, "csv");
  csvp->field_line = 1;
  csvp->lower_case = 1;
  csvp->max_nr_fields = 512;
  csvp->nr_fields = 0;
  /* csvp->field_names = 0; */ /*nmem_strdup(csvp->nmem, "a|b|c|d|e");*/

  csvp->field_name 
      = nmem_malloc(csvp->nmem, 
		    sizeof(*(csvp->field_name)) * csvp->max_nr_fields);
  for (i = 0; i < csvp->max_nr_fields; i++){
    csvp->field_name[i] = 0; 
  }

  /* know field names from config file */
  /*if (strlen(csvp->field_names))
    yaz_log (YLOG_LOG, "CSV filter grs_config_csv field names");
  */

  yaz_log (YLOG_LOG, "Ended CSV filter grs_config_csv");
  return ZEBRA_OK;
}


static data1_node *grs_read_csv (struct grs_read_info *gri)
{
  data1_node *root_node = 0;
  data1_node *node = 0;
  struct csv_t *csvp = (struct csv_t *)gri->clientData;
  int field_nr = 0; 
  int end_of_record = 0;
  int read_header = 0;
  int read_bytes = 0;
  char *cb = csvp->buf;
  char *cv = csvp->value; 

  yaz_log (YLOG_LOG, "Called CSV filter grs_read_csv");

  /* if on start of first line, read header line for dynamic configure */ 
  if(csvp->field_line && gri->offset == 0)
    read_header = 1;

  while (!end_of_record){

#if 0    
    /* configure grs.csv filter with first line in file containing field 
       name information */
    if (read_header){
      yaz_log (YLOG_LOG, "CSV filter grs_read_csv reading header line");
      
      /* create new memory for fieldname and value */
      if (old_nr_fields < csvp->nr_fields){
        yaz_log(YLOG_LOG, 
                "CSV filter grs_read_csv name:'%d' ", csvp->nr_fields);
        old_nr_fields = csvp->nr_fields;
        csvp->field_name[csvp->nr_fields] 
          = nmem_malloc(csvp->nmem, csvp->name_size);
        csvp->field_value[csvp->nr_fields] 
          = nmem_malloc(csvp->nmem, csvp->value_size);

        /* read buf and copy values to field_name[] */  
        read_bytes = (*gri->readf)(gri->fh, csvp->buf, csvp->buf_size);
       gri-> offset = (*gri->tellf)(gri->fh);
        /* yaz_log(YLOG_LOG, "CSV filter grs_read_csv offset:'%d' ", offset); */
        read_header = 0;
      }
    } else {
      /* read buf and copy values to field_value[] */  
      read_bytes = (*gri->readf)(gri->fh, csvp->buf, csvp->buf_size);
      gri->offset = (*gri->tellf)(gri->fh);
      yaz_log(YLOG_LOG, "CSV filter grs_read_csv offset:'%d' ", offset);
    }
    
#endif


    /* read new buffer from file */  
    read_bytes = (*gri->readf)(gri->fh, csvp->buf, csvp->buf_size);

    yaz_log (YLOG_LOG, "CSV filter grs_read_csv read_bytes  %d", read_bytes);
    yaz_log (YLOG_LOG, "CSV filter grs_read_csv csvp->buf %s", csvp->buf);

    gri->offset = (*gri->tellf)(gri->fh);
    yaz_log(YLOG_LOG, "CSV filter grs_read_csv gri->offset:'%d' ", 
            (int)gri->offset);

    /* work on buffer */
    cb = csvp->buf;
    while ((cb - csvp->buf < read_bytes)
           && (cv - csvp->value < csvp->value_size)
           && !end_of_record){

      if (*cb == csvp->field_char){
        /* if field finished */
        *cv = '\0';
        if (read_header){
          /* read field names from header line */
          if (csvp->nr_fields < csvp->max_nr_fields){
            csvp->field_name[csvp->nr_fields] 
              = nmem_strdup(csvp->nmem, csvp->value);
            
            csvp->nr_fields++;
            yaz_log (YLOG_LOG, "CSV filter grs_read_csv field %d name '%s'", 
                       field_nr, csvp->value);
          } else {
            yaz_log (YLOG_WARN, "CSV filter grs_read_csv field %d name '%s' "
                     "exceeds configured max number of fields %d", 
                     field_nr, csvp->value, csvp->max_nr_fields);
          }
        } else {
          /* process following value line fields */
          if (field_nr < csvp->nr_fields){
            /* less or qual fields number */
            yaz_log (YLOG_LOG, "CSV filter grs_read_csv field %d %s: '%s'", 
                     field_nr, csvp->field_name[field_nr], csvp->value);
          } else {
          /* too many fields */
            yaz_log (YLOG_WARN, "CSV filter grs_read_csv field value %d %s "
                     "exceeds dynamic configured number of fields %d", 
                     field_nr, csvp->value, csvp->nr_fields);
          }
          
        }
        /* advance buffer and proceed to next field */
        cb++;
        cv = csvp->value;
        field_nr++;
      } else if (*cb == csvp->record_char){
        /* if record finished */
        /* advance buffer and proceed to record */
        *cv = '\0';
        cb++;
        cv = csvp->value;
        field_nr = 0;
        if (read_header){
          read_header = 0;
          yaz_log (YLOG_LOG, "CSV filter grs_read_csv header end");
        } else {
          end_of_record = 1;
          yaz_log (YLOG_LOG, "CSV filter grs_read_csv record end");
        }
      } else {
        /* just plain char to be stored in value, no special action at all */
        if (csvp->lower_case && read_header){
          *cv = tolower(*cb);
        } else {
          *cv = *cb;
        }
         cb++;
         cv++;
      }
    }
  
      
    /* if (gri->endf)
      (*gri->endf)(gri->fh, offset - 1);  */
  }

  /* try to build GRS node and document */
  
  root_node = data1_mk_root(gri->dh, gri->mem, csvp->root_element);
  node = data1_mk_node2(gri->dh, gri->mem, DATA1N_data, root_node);
  node = data1_mk_tag(gri->dh, gri->mem, "pr_name_gn", 0, node);  
  data1_mk_text_n(gri->dh, gri->mem, csvp->buf, read_bytes, node);
  
  if (!root_node){
    yaz_log (YLOG_WARN, "empty CSV record of type '%s' "
             "near file offset %d "
             "or missing abstract syntax file '%s.abs'",
             csvp->root_element, (int)gri->offset, csvp->root_element);
    return 0;
  }

  yaz_log (YLOG_LOG, "Ended CSV filter grs_read_csv");
  return root_node;
}

static void *grs_init_csv(Res res, RecType recType)
{
  NMEM m = nmem_create();
  struct csv_t *csvp = (struct csv_t *) nmem_malloc(m, sizeof(*csvp));
  yaz_log (YLOG_LOG, "Called CSV filter grs_init_csv");
  csvp->nmem = m;
  yaz_log (YLOG_LOG, "Ended CSV filter grs_init_csv");
  return csvp;
}

static void grs_destroy_csv(void *clientData)
{
  struct csv_t *csvp = (struct csv_t*) clientData;

  yaz_log (YLOG_LOG, "Called CSV filter grs_destroy_csv");

  nmem_destroy(csvp->nmem);
  clientData = 0;

  yaz_log (YLOG_LOG, "Ended CSV filter grs_destroy_csv");
}

static int grs_extract_csv(void *clientData, struct recExtractCtrl *ctrl)
{
  int res;
  /* struct csv_t *csvp = (struct csv_t*) clientData; */

  yaz_log (YLOG_LOG, "Called CSV filter grs_extract_csv");
  yaz_log (YLOG_LOG, "recExtractCtr fh     %d", (int)ctrl->fh);
  yaz_log (YLOG_LOG, "recExtractCtr offset %d", (int)ctrl->offset);

  res = zebra_grs_extract(clientData, ctrl, grs_read_csv);

  yaz_log (YLOG_LOG, "recExtractCtr fh     %d", (int)ctrl->fh);
  yaz_log (YLOG_LOG, "recExtractCtr offset %d", (int)ctrl->offset);
  yaz_log (YLOG_LOG, "Ended CSV filter grs_extract_csv");

  return res;
}

static int grs_retrieve_csv(void *clientData, struct recRetrieveCtrl *ctrl)
{
  int res;
  /* struct csv_t *csvp = (struct csv_t*) clientData; */
  
  yaz_log (YLOG_LOG, "Called CSV filter grs_retrieve_csv");
  res = zebra_grs_retrieve(clientData, ctrl, grs_read_csv);
  yaz_log (YLOG_LOG, "Ended CSV filter grs_retrieve_csv");

  return res;
}

static struct recType grs_type_csv =
{
    0,
    "grs.csv",
    grs_init_csv,
    grs_config_csv,
    grs_destroy_csv,
    grs_extract_csv,
    grs_retrieve_csv
};

RecType
#ifdef IDZEBRA_STATIC_GRS_CSV
idzebra_filter_grs_csv
#else
idzebra_filter
#endif

[] = {
    &grs_type_csv,
    0,
};

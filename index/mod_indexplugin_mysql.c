/* This file is part of the Zebra server.
   Copyright (C) 1994-2009 Index Data

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

#include "indexplugin.h"
#include <mysql/mysql.h>
#include <stdint.h>

MYSQL mCon;

static int mysqlConnect(const char *username, const char *password, const char *database, const char *hostname)
{
    mysql_init(&mCon);

    /* Set the default encoding to utf-8 so that zebra 
       doesn't gribe that the XML conflicts with it's encoding */
    mysql_options(&mCon, MYSQL_SET_CHARSET_NAME, "utf8");

    mysql_options(&mCon, MYSQL_READ_DEFAULT_GROUP, "indexplugin_mysql");
    if (!mysql_real_connect(&mCon, hostname, username, password, database, 0, NULL, 0))
    {
        yaz_log(YLOG_FATAL, "Failed to connect to database: %s\n", mysql_error(&mCon));
        return ZEBRA_FAIL;
    }
    else
    {
        yaz_log(YLOG_LOG, "Connected to Mysql Database");
    }
	
    return ZEBRA_OK;
}


static int repositoryExtract(ZebraHandle zh, const char *driverCommand, enum zebra_recctrl_action_t action)
{
    /* this doesn't really need to be initialised */
    int ret = ZEBRA_FAIL;

    //nasty parsing method
    char *sqlQuery = strchr(driverCommand, ':');
    if(sqlQuery) *(sqlQuery ++) = NULL;
    else
    {
        yaz_log(YLOG_LOG, "No MySQL Query given, falling back on config default");
        sqlQuery = res_get_named(zh->session_res, "indexplugin.mysql_defaultsql", driverCommand);
    }

    yaz_log(YLOG_LOG, "Database configuration selected: %s", driverCommand);

    //Get our connection specific info from the config
    //TODO: make the "test" bit configurable by command line
    const char *username = res_get_named(zh->session_res, "indexplugin.mysql_username", driverCommand);
    const char *password = res_get_named(zh->session_res, "indexplugin.mysql_password", driverCommand);
    const char *hostname = res_get_named(zh->session_res, "indexplugin.mysql_hostname", driverCommand);
    const char *database = res_get_named(zh->session_res, "indexplugin.mysql_database", driverCommand);

    const char *idfield = res_get_named(zh->session_res, "indexplugin.mysql_idfield", driverCommand);
    const char *datafield = res_get_named(zh->session_res, "indexplugin.mysql_datafield", driverCommand);

    if(!username) 
    {
        yaz_log(YLOG_FATAL, "Database configuration incomplete or missing");
        return ZEBRA_FAIL;
    }

    if(!sqlQuery)
    {
        yaz_log(YLOG_FATAL, "No valid MySQL query");
        return ZEBRA_FAIL;
    }

    enum
    {
        IDFIELD,
        DATAFIELD
    };
    //This is a rudimentary way of binding fields, it's nasty
    uint8_t fieldBind[2] = {0xFF, 0xFF};

    yaz_log(YLOG_LOG, "MySQL Query: %s", sqlQuery);

    if ((ret = mysqlConnect(username, password, database, hostname)) == ZEBRA_OK)
    {
        const char *mQuery = sqlQuery;
        if (mysql_real_query(&mCon, mQuery, strlen(mQuery)) == 0)
        {
            MYSQL_RES *result = NULL;
            if ((result = mysql_store_result(&mCon)))
            {
                //Check for the binding fields
                MYSQL_FIELD *field;
                int i = 0;
                while(field = mysql_fetch_field(result))
                {
                    if(strcmp(field->name, idfield) == 0) fieldBind[IDFIELD] = i;
                    if(strcmp(field->name, datafield) == 0) fieldBind[DATAFIELD] = i;
                    i ++;
                }

                //Test the binding fields
                if(fieldBind[IDFIELD] == 0xFF || fieldBind[DATAFIELD] == 0xFF)
                {
                    yaz_log(YLOG_FATAL, "Query did not reveal all/any binding columns");
                    ret = ZEBRA_FAIL;
                }
                else
                {
                    yaz_log(YLOG_LOG, "Successfully found all binding columns");
                        
                    unsigned int num_fields;
                    num_fields = mysql_num_fields(result);

                    MYSQL_ROW row;
                    while ((row = mysql_fetch_row(result)))
                    {
                        unsigned long *lengths;
                        lengths = mysql_fetch_lengths(result);

                        //This is the critical line, that actually indexes your data
                        //Args: Zebra Handle, Data, Data length, Action, FileName(Unique identifier)
                        zebraIndexBuffer(zh, row[fieldBind[DATAFIELD]], lengths[fieldBind[DATAFIELD]], action, row[fieldBind[IDFIELD]]);
                    }
                }
                mysql_free_result(result);
            }
        }
        else
        {
            yaz_log(YLOG_FATAL, "Failed to run query: %s", mysql_error(&mCon));
            ret = ZEBRA_FAIL;
        }
    }

    /* Drop our MYSQL connection as we don't need it anymore
       and deallocate anything allocated */
    mysql_close(&mCon);

    return ret;
}

void indexPluginRegister(void)
{
    /* register our function that gets called while indexing a document */
    addDriverFunction(repositoryExtract);
}
/*
 * Local variables:
 * c-basic-offset: 4
 * c-file-style: "Stroustrup"
 * indent-tabs-mode: nil
 * End:
 * vim: shiftwidth=4 tabstop=8 expandtab
 */


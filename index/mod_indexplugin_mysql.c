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

MYSQL mCon;

static int mysqlConnect(void)
{
    mysql_init(&mCon);

    /* Set the default encoding to utf-8 so that zebra 
       doesn't gribe that the XML conflicts with it's encoding */
    mysql_options(&mCon, MYSQL_SET_CHARSET_NAME, "utf8");

    mysql_options(&mCon, MYSQL_READ_DEFAULT_GROUP, "indexplugin_mysql");
    if (!mysql_real_connect(&mCon, "127.0.0.1", "test", "test", "newDatabase", 0, NULL, 0))
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

    assert(driverCommand);

    yaz_log(YLOG_LOG, "Driver command: %s", driverCommand);

    if ((ret = mysqlConnect()) == ZEBRA_OK)
    {
        const char *mQuery = driverCommand;
        if (mysql_real_query(&mCon, mQuery, strlen(mQuery)) == 0)
        {
            MYSQL_RES *result;
            if ((result = mysql_store_result(&mCon)))
            {
                MYSQL_ROW row;
                unsigned int num_fields;
			
                num_fields = mysql_num_fields(result);
                while ((row = mysql_fetch_row(result)))
                {
                    unsigned long *lengths;
                    lengths = mysql_fetch_lengths(result);

                    zebraIndexBuffer(zh, row[1], lengths[1], action, row[0]);
                }
                mysql_free_result(result);
            }
        }
        else
        {
            yaz_log(YLOG_FATAL, "Failed to run query: %s\n", mysql_error(&mCon));
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


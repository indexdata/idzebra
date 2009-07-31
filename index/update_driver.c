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

#include <stdio.h>
#include <assert.h>
#include <sys/types.h>
#ifdef WIN32
#include <io.h>
#define S_ISREG(x) (x & _S_IFREG)
#define S_ISDIR(x) (x & _S_IFDIR)
#endif
#if HAVE_UNISTD_H
#include <unistd.h>
#endif
#include <direntz.h>
#include <fcntl.h>
#include <time.h>

#include "index.h"

/* plugin includes */
#include <sys/stat.h>
#include "indexplugin.h"
#include <stdlib.h>
#include <dlfcn.h>





zebra_index_plugin_object *pluginObj = NULL;

static zebra_index_plugin_object *newZebraPlugin(void)
{
    zebra_index_plugin_object *newPlugin = malloc(sizeof(zebra_index_plugin_object));
    return newPlugin;
}

static void destroyZebraPlugin(zebra_index_plugin_object *zebraIdxPlugin)
{
    free(zebraIdxPlugin);
}

void addDriverFunction(indexList function)
{
    /* Assign the function to the object */
    pluginObj->idxList = function;
}


void zebraIndexBuffer(ZebraHandle zh, char *data, int dataLength, enum zebra_recctrl_action_t action, char *name)
{
    zebra_buffer_extract_record(zh, data, dataLength, action, zh->m_record_type, NULL, NULL, name);
}


/* I'm not even sure what this is for */
void repositoryShowDriver(ZebraHandle zh, const char *path)
{
    char src[1024];
    int src_len;
    struct dirs_entry *dst;
    Dict dict;
    struct dirs_info *di;

    if (!(dict = dict_open_res(zh->reg->bfs, FMATCH_DICT, 50, 0, 0, zh->res)))
    {
        yaz_log(YLOG_FATAL, "dict_open fail of %s", FMATCH_DICT);
	return;
    }
    
    strncpy(src, path, sizeof(src)-1);
    src[sizeof(src)-1]='\0';
    src_len = strlen(src);
    
    if (src_len && src[src_len-1] != '/')
    {
        src[src_len] = '/';
        src[++src_len] = '\0';
    }
    
    di = dirs_open(dict, src, zh->m_flag_rw);
    
    while ((dst = dirs_read(di)))
        yaz_log(YLOG_LOG, "%s", dst->path);
    dirs_free(&di);
    dict_close(dict);
}


ZEBRA_RES zebra_update_from_driver(ZebraHandle zh, const char *path, 
                                   enum zebra_recctrl_action_t action, char *useIndexDriver)
{
    /* delcair something to hold out remote call */
    void (*idxPluginRegister)(void);
    char *dlError;
    void *libHandle;
    int pluginReturn;

    char driverName[100];
    sprintf(driverName, "mod-%s.so", useIndexDriver);

    yaz_log(YLOG_LOG, "Loading driver %s", useIndexDriver);

    libHandle = dlopen(driverName, RTLD_LAZY);
    if (!libHandle)
    {
        yaz_log(YLOG_FATAL, "Unable to load index plugin %s", dlerror());
        return ZEBRA_FAIL;
    }
    /* clear the error buffer */
    dlerror();

    idxPluginRegister = dlsym(libHandle, "indexPluginRegister");

    if ((dlError = dlerror()) != NULL)
    {
        yaz_log(YLOG_FATAL, "Index plugin error: %s", dlError);

        /* Although the documentation says this dlclose isn't needed 
           it seems better to put it in, incase there were memory
           allocations */
        dlclose(libHandle);
        return ZEBRA_FAIL;
    }

    pluginObj = newZebraPlugin();
    
    /* invoke the plugin starter */
    idxPluginRegister();

    pluginReturn = pluginObj->idxList(zh, path, action);
    destroyZebraPlugin(pluginObj);
    
    /* close the plugin handle */
    dlclose(libHandle);

    /* repositoryExtract(zh, path, action);*/
    return pluginReturn;
}

/*
 * Local variables:
 * c-basic-offset: 4
 * c-file-style: "Stroustrup"
 * indent-tabs-mode: nil
 * End:
 * vim: shiftwidth=4 tabstop=8 expandtab
 */


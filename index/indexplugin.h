#ifndef INDEXPLUGINH
#define INDEXPLUGINH

#include "../index/index.h"
#include <assert.h>


typedef int (*indexList)(ZebraHandle zh, const char *driverArg, enum zebra_recctrl_action_t action);

typedef struct 
{
	indexList idxList;
} zebra_index_plugin_object;

void addDriverFunction(indexList);
void zebraIndexBuffer(ZebraHandle zh, char *data, int dataLength, enum zebra_recctrl_action_t action, char *name);

#endif

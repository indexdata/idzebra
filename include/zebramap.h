/*
 * Copyright (C) 1994-1998, Index Data I/S 
 * All rights reserved.
 * Sebastian Hammer, Adam Dickmeiss
 *
 * $Log: zebramap.h,v $
 * Revision 1.6  1998-06-23 15:33:33  adam
 * Added feature to specify sort criteria in query (type 7 specifies
 * sort flags).
 *
 * Revision 1.5  1998/03/05 08:39:26  adam
 * Minor changes to zebramap data structures. Changed query
 * mapping rules.
 *
 * Revision 1.4  1998/02/10 12:03:05  adam
 * Implemented Sort.
 *
 * Revision 1.3  1997/11/18 10:05:08  adam
 * Changed character map facility so that admin can specify character
 * mapping files for each register type, w, p, etc.
 *
 * Revision 1.2  1997/10/29 12:02:47  adam
 * Added missing prototype.
 *
 * Revision 1.1  1997/10/27 14:33:04  adam
 * Moved towards generic character mapping depending on "structure"
 * field in abstract syntax file. Fixed a few memory leaks. Fixed
 * bug with negative integers when doing searches with relational
 * operators.
 *
 */

#ifndef ZEBRAMAP_H
#define ZEBRAMAP_H

#include <proto.h>
#include <res.h>

#ifdef __cplusplus
extern "C" {
#endif

typedef struct zebra_maps *ZebraMaps;
ZebraMaps zebra_maps_open (Res res);

void zebra_maps_close (ZebraMaps zm);

const char **zebra_maps_input (ZebraMaps zms, unsigned reg_id,
			       const char **from, int len);
const char *zebra_maps_output(ZebraMaps, unsigned reg_id, const char **from);

int zebra_maps_attr (ZebraMaps zms, Z_AttributesPlusTerm *zapt,
		     unsigned *reg_id, char **search_type, char **rank_type,
		     int *complete_flag, int *sort_flag);

int zebra_maps_sort (ZebraMaps zms, Z_SortAttributes *sortAttributes);

int zebra_maps_is_complete (ZebraMaps zms, unsigned reg_id);
int zebra_maps_is_sort (ZebraMaps zms, unsigned reg_id);
#ifdef __cplusplus
}
#endif

#endif

/*
 * Copyright (C) 1994-1997, Index Data I/S 
 * All rights reserved.
 * Sebastian Hammer, Adam Dickmeiss
 *
 * $Log: zebramap.h,v $
 * Revision 1.1  1997-10-27 14:33:04  adam
 * Moved towards generic character mapping depending on "structure"
 * field in abstract syntax file. Fixed a few memory leaks. Fixed
 * bug with negative integers when doing searches with relational
 * operators.
 *
 */

#ifndef ZEBRAMAP_H
#define ZEBRAMAP_H

#include <proto.h>

#ifdef __cplusplus
extern "C" {
#endif

typedef struct zebra_maps *ZebraMaps;
ZebraMaps zebra_maps_open (const char *tabpath);

void zebra_maps_close (ZebraMaps zm);

const char **zebra_maps_input (ZebraMaps zms, int reg_type,
			       const char **from, int len);
const char *zebra_maps_output(ZebraMaps, int reg_type, const char **from);

int zebra_maps_attr (ZebraMaps zms, Z_AttributesPlusTerm *zapt,
		     int *reg_type, char **search_type, int *complete_flag);

#ifdef __cplusplus
}
#endif

#endif

#ifndef INLINE_H
#define INLINE_H

#include "marcomp.h"

#ifdef __cplusplus
extern "C" {
#endif


typedef struct inline_field
{
    char *name;
    char *ind1;
    char *ind2;
    struct inline_subfield *list;
} inline_field;
typedef struct inline_subfield
{
    char *name;
    char *data;
    struct inline_subfield *next;
    struct inline_subfield *parent;
} inline_subfield;

inline_field *inline_parse(const char *s);
void inline_destroy_field(inline_field *p);

#ifdef __cplusplus
}
#endif

#endif

/* Moved from zrpn.c -pop */

#ifndef ZEBRA_XPATH_H
#define ZEBRA_XPATH_H

#include <yaz/nmem.h>

#define XPATH_STEP_COUNT 10
struct xpath_predicate {
    int which;
    union {
#define XPATH_PREDICATE_RELATION 1
        struct {
            char *name;
            char *op;
            char *value;
        } relation;
#define XPATH_PREDICATE_BOOLEAN 2
        struct {
            const char *op;
            struct xpath_predicate *left;
            struct xpath_predicate *right;
        } boolean;
    } u;
};

struct xpath_location_step {
    char *part;
    struct xpath_predicate *predicate;
};

int zebra_parse_xpath_str(const char *xpath_string,
                          struct xpath_location_step *xpath,
                          int max, NMEM mem);

void dump_xp_steps (struct xpath_location_step *xpath, int no);


#endif

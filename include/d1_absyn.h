
#ifndef D1_ABSYN_H
#define D1_ABSYN_H 1

#define ENHANCED_XELM 1

#include <zebra_xpath.h>
#include <idzebra/data1.h>
#include <dfa.h>

typedef struct data1_xpelement
{
    char *xpath_expr;
#ifdef ENHANCED_XELM 
    struct xpath_location_step xpath[XPATH_STEP_COUNT];
    int xpath_len;
#endif
    struct DFA *dfa;  
    data1_termlist *termlists;
    struct data1_xpelement *next;
} data1_xpelement;

struct data1_absyn
{
    char *name;
    oid_value reference;
    data1_tagset *tagset;
    data1_attset *attset;
    data1_varset *varset;
    data1_esetname *esetnames;
    data1_maptab *maptabs;
    data1_marctab *marc;
    data1_sub_elements *sub_elements;
    data1_element *main_elements;
    struct data1_xpelement *xp_elements; /* pop */
    struct data1_systag *systags;
    char *encoding;
    int  enable_xpath_indexing;
};

#endif

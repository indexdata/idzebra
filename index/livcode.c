/*
  
The University of Liverpool

Modifications to Zebra 1.1 / YAZ 1.7 to enable ranking
by attribute weight.

Copyright (c) 2001-2002 The University of Liverpool.  All
rights reserved.

Licensed under the Academic Free License version 1.1.
http://opensource.org/licenses/academic.php

$Id: livcode.c,v 1.2 2004-08-04 08:35:23 adam Exp $

*/

#include <stdlib.h>
#include <stdio.h>
#ifdef WIN32
#include <process.h>
#else
#include <unistd.h>
#endif
#include <assert.h>

#include "index.h"
#include "zserver.h"

/*
** These functions/routines 
** 1. reads in and builds a linked list of rank attr/rank score pairs
** 2. expand a simple query into a paired list of complex/simple nodes.
*/

typedef struct rstype
{
    struct rstype *next_rsnode ;
    int rank ;
    int score ;
    char *rankstr ;
} rsnode, *refrsnode ;

refrsnode start_rsnode = NULL ;

/*
** Function/Routine prototypes
*/
static int        search_for_score( char *rankstr ) ;
static char      *search_for_rankstr( int rank ) ;
static int        search_for_rank( int rank ) ;
static refrsnode  set_rsnode( int rank, int score ) ;
static int        read_zrank_file(ZebraHandle zh) ;

static void       convert_simple2complex(ZebraHandle zh, Z_RPNStructure *rpnstruct ) ;
static void       walk_complex_query(ZebraHandle zh, Z_RPNStructure *rpnstruct ) ;
static Z_Complex *expand_query(ZebraHandle zh, Z_Operand *thisop ) ;
static Z_Complex *set_1complex_1operand( Z_Complex *comp,Z_Operand *simp ) ;
static Z_Complex *set_2operands( Z_Operand *sim1,Z_Operand *sim2 ) ;
static Z_Operand *set_operand( Z_Operand *thisop, int newattr ) ;
static int        check_operand_attrs( Z_Operand *thisop ) ;

/*
** search_for_score()
** given a rank-string traverse down the linked list ;
** return its score if found otherwise return -1.
*/
int search_for_score( char *rankstr )
{
    refrsnode node = start_rsnode ;
    int rank ;
  
    if ( sscanf( rankstr,"%d",&rank ) )
    {
        while ( node )
        {
            if ( node->rank == rank ) return node->score ;
            node = node->next_rsnode ;
        }
    }
    return -1 ;
}

/*
** search_for_rankstr()
** given a rank traverse down the linked list ; 
** return its string if found otherwise return NULL.
*/
char *search_for_rankstr( int rank )
{
    refrsnode node = start_rsnode ;
  
    while ( node )
    {
        if ( node->rank == rank ) return node->rankstr ;
        node = node->next_rsnode ;
    }
    return "rank" ;
}

/*
** search_for_rank()
** given a rank traverse down the linked list ;
** return 1 if found otherwise return 0.
*/
int search_for_rank( int rank )
{
    refrsnode node = start_rsnode ;

    while ( node )
    {
        if ( node->rank == rank ) return 1 ;
        node = node->next_rsnode ;
    }
    return 0 ;
}

/*
** set_rsnode()
** given a rank and a score, build the rest of the rsnode structure.
*/
refrsnode set_rsnode( int rank, int score )
{
#define BUFFMAX 128
    refrsnode node = (refrsnode)malloc( sizeof(rsnode) ) ;
    char buff[BUFFMAX] ;

    node->next_rsnode = NULL  ;
    node->rank        = rank  ;
    node->score       = score ;

    sprintf( buff,"%d",rank ) ;
    node->rankstr     = (char *)malloc( strlen(buff)+1 ) ;
    strcpy( node->rankstr, buff ) ;

    return node ;
}

/*
** read_zrank_file(zh)
** read in the rankfile and build the rank/score linked list ; 
** return 0 : can't open the zebra config. file
** return 0 : can't find the rankfile entry in the zebra config. file
** return 0 : can't open the rankfile itself
** return the number of distinct ranks read in.
*/
int read_zrank_file(ZebraHandle zh)
{
#define LINEMAX 256
    char line[ LINEMAX ] ;
    char rname[ LINEMAX ] ;
    char *lineptr ;
    FILE *ifd ;
    int rank = 0 ;
    int score = 0 ;
    int numranks = 0 ;
 
    /*
    ** open the zebra configuration file and look for the "rankfile:"
    ** entry which contains the path/name of the rankfile
    */
    
    const char *rankfile = res_get_def(zh->res, "rankfile", 0);
    const char *profilePath = res_get_def(zh->res, "profilePath",
                                          DEFAULT_PROFILE_PATH);

    if (!rankfile)
    {
        yaz_log(LOG_LOG, "rankfile entry not found in config file" ) ;
        return 0 ;
    }
    ifd = yaz_path_fopen(profilePath, rankfile, "r" ) ;
    if ( ifd )
    {
        while ( (lineptr = fgets( line,LINEMAX,ifd )) )
        {
            if ( sscanf( lineptr,"rankfile: %s", rname ) == 1 )
                rankfile  = rname ;
        }

        /*
        ** open the rankfile and read the rank/score pairs
        ** ignore 1016 
        ** ignore duplicate ranks 
        ** ignore ranks without +ve scores
        */
        if ( rankfile )
        { 
            if ( !(ifd = fopen( rankfile, "r" )) )
            {
                logf( LOG_LOG, "unable to open rankfile %s",rankfile ) ;
                return 0;
            }
     
            while ( (lineptr = fgets( line,LINEMAX,ifd )) )
            {
                sscanf( lineptr,"%d : %d", &rank,&score ) ;
                if ( ( score > 0 ) && ( rank != 1016 ) )
                {
                    refrsnode new_rsnode ;
    
                    if ( search_for_rank( rank ) == 0 )
                    {
                        new_rsnode              = set_rsnode( rank,score ) ;
                        new_rsnode->next_rsnode = start_rsnode ;
                        start_rsnode            = new_rsnode ;
                        numranks++ ;
                    }
                }
            }
        }
        else 
        {
            yaz_log(LOG_WARN|LOG_ERRNO, "unable to open config file (%s)",
                    rankfile);
        }
    }
    return numranks ;
}

/*
** set_operand() 
** build an operand "node" - hav to make a complete copy of thisop and 
** then insert newattr in the appropriate place
** 
*/
Z_Operand *set_operand( Z_Operand *thisop, int newattr )
{
    Z_Operand            *operand ;
    Z_AttributesPlusTerm *attributesplusterm ;
    Z_AttributeList      *attributelist ;
    Z_AttributeElement   *attributeelement ;
    Z_AttributeElement   *attrptr ;
    Z_AttributeElement   **attrptrptr ;
    Z_Term               *term ;
    Odr_oct              *general ;
    int i ;

    operand            = (Z_Operand *) 
                         malloc( sizeof( Z_Operand ) ) ;
    attributesplusterm = (Z_AttributesPlusTerm *)
                         malloc( sizeof( Z_AttributesPlusTerm ) ) ;
    attributelist      = (Z_AttributeList *)
                         malloc( sizeof( Z_AttributeList ) ) ;
    attributeelement   = (Z_AttributeElement *)
                         malloc( sizeof( Z_AttributeElement ) ) ;
    term               = (Z_Term *)
                         malloc( sizeof( Z_Term ) ) ;
    general            = (Odr_oct *)
                         malloc( sizeof( Odr_oct ) ) ;

    operand->which                 = Z_Operand_APT ;
    operand->u.attributesPlusTerm  = attributesplusterm ;

    attributesplusterm->attributes = attributelist ;
    attributesplusterm->term       = term ;

    attributelist->num_attributes  = thisop->u.attributesPlusTerm->
                                     attributes->num_attributes ;

    attrptr = (Z_AttributeElement *) malloc( sizeof(Z_AttributeElement) *
                                             attributelist->num_attributes ) ;
    attrptrptr = (Z_AttributeElement **) malloc( sizeof(Z_AttributeElement) *
                                           attributelist->num_attributes ) ; 

    attributelist->attributes = attrptrptr ;

    for ( i = 0 ; i < attributelist->num_attributes ; i++ )
    {
        *attrptr = *thisop->u.attributesPlusTerm->attributes->attributes[i] ;

         attrptr->attributeType  = (int *)malloc( sizeof(int *) ) ;
        *attrptr->attributeType  = *thisop->u.attributesPlusTerm->attributes->
                                    attributes[i]->attributeType;

         attrptr->value.numeric  = (int *)malloc( sizeof(int *) ) ;
        *attrptr->value.numeric  = *thisop->u.attributesPlusTerm->attributes->
                                    attributes[i]->value.numeric;

        if ( (*attrptr->attributeType == 1) && 
             (*attrptr->value.numeric == 1016) )
        {
            *attrptr->value.numeric = newattr ;
        }
        *attrptrptr++ = attrptr++ ;
    }

    term->which     = Z_Term_general ;
    term->u.general = general ;

    general->len  = thisop->u.attributesPlusTerm->term->u.general->len ;
    general->size = thisop->u.attributesPlusTerm->term->u.general->size ;
    general->buf  = malloc( general->size ) ;
    strcpy( general->buf,
            thisop->u.attributesPlusTerm->term->u.general->buf ) ;

    return operand ;
}

/*
** set_2operands()
** build a complex "node" with two (simple) operand "nodes" as branches
*/
Z_Complex *set_2operands( Z_Operand *sim1,Z_Operand *sim2 )
{
    Z_Complex *top        ;
    Z_RPNStructure *s1    ;
    Z_RPNStructure *s2    ;
    Z_Operator *roperator ;

    top       = (Z_Complex *)     malloc( sizeof( Z_Complex      ) ) ;
    s1        = (Z_RPNStructure *)malloc( sizeof( Z_RPNStructure ) ) ;
    s2        = (Z_RPNStructure *)malloc( sizeof( Z_RPNStructure ) ) ;
    roperator = (Z_Operator *)    malloc( sizeof( Z_Operator     ) ) ;

    top->roperator          = roperator     ;
    top->roperator->which   = Z_Operator_or ;
    top->roperator->u.op_or = odr_nullval() ;

    top->s1                 = s1                    ;
    top->s1->which          = Z_RPNStructure_simple ;
    top->s1->u.simple       = sim1                  ;

    top->s2                 = s2                    ;
    top->s2->which          = Z_RPNStructure_simple ;
    top->s2->u.simple       = sim2                  ;

    return top ;
}

/*
** set_1complex_1operand()
** build a complex "node" with a complex "node" branch and an 
** operand "node" branch
*/
Z_Complex *set_1complex_1operand( Z_Complex *comp,Z_Operand *simp )
{
    Z_Complex *top        ;
    Z_RPNStructure *s1    ;
    Z_RPNStructure *s2    ;
    Z_Operator *roperator ;

    top       = (Z_Complex *)     malloc( sizeof( Z_Complex      ) ) ;
    s1        = (Z_RPNStructure *)malloc( sizeof( Z_RPNStructure ) ) ;
    s2        = (Z_RPNStructure *)malloc( sizeof( Z_RPNStructure ) ) ;
    roperator = (Z_Operator *)    malloc( sizeof( Z_Operator     ) ) ;

    top->roperator          = roperator     ;
    top->roperator->which   = Z_Operator_or ;
    top->roperator->u.op_or = odr_nullval() ;

    top->s1                 = s1                     ;
    top->s1->which          = Z_RPNStructure_complex ;
    top->s1->u.complex      = comp                   ;

    top->s2                 = s2                     ;
    top->s2->which          = Z_RPNStructure_simple  ;
    top->s2->u.simple       = simp                   ;

    return top ;
}

/*
** expand_query()
** expand a simple query into a number of complex queries
*/
Z_Complex *expand_query(ZebraHandle zh, Z_Operand *thisop )
{
    Z_Complex *top ;
    int numattrs = 0 ;

    /*
    ** start_rsnode will be set if we have already read the rankfile 
    ** so don't bother again but we need to know the number of attributes
    ** in the linked list so traverse it again to find out how many.
    */
    if ( start_rsnode )
    {
        refrsnode node = start_rsnode ;
        while ( node )
        {
            numattrs++ ;
            node = node->next_rsnode ;
        }
    } 

    /*
    ** only expand the query if there are 2 or more attributes 
    */
    if ( numattrs >= 2 )
    {
        refrsnode node = start_rsnode ;
        int attr1 ;
        int attr2 ;

        attr1 = node->rank ; node = node->next_rsnode ;
        attr2 = node->rank ; node = node->next_rsnode ;

        /*
        ** this is the special case and has to be done first because the 
        ** last complex node in the linear list has two simple nodes whereas 
        ** all the others have a complex and a simple.
        */
        top = set_2operands( set_operand( thisop,attr1 ),
                             set_operand( thisop,attr2 ) ) ;

        /*
        ** do the rest as complex/simple pairs
        */
        while ( node )
        {
            attr1 = node->rank ; node = node->next_rsnode ;
            top = set_1complex_1operand( top,set_operand( thisop,attr1 ) ) ;
        }
        /*
        ** finally add the 1016 rank attribute at the top of the tree
        */
        top = set_1complex_1operand( top,set_operand( thisop,1016 ) ) ;

        return top ;
    }
    else return NULL ;
}

/*
** check_operand_attrs()
** loop through the attributes of a particular operand 
** return 1 if (type==1 && value==1016) && (type==2 && value==102) 
** otherwise return 0
*/
int check_operand_attrs( Z_Operand *thisop ) 
{
    Z_AttributeElement *attrptr ;
    int cond1 = 0 ;
    int cond2 = 0 ;
    int numattrs ;
    int i ;

    numattrs = thisop->u.attributesPlusTerm->attributes->num_attributes ;

    for ( i = 0 ; i < numattrs ; i++ )
    {
        attrptr = thisop->u.attributesPlusTerm->attributes->attributes[i] ;

        if ( (*attrptr->attributeType == 1) && 
             (*attrptr->value.numeric == 1016) )
            cond1 = 1 ;

        if ( (*attrptr->attributeType == 2) && 
             (*attrptr->value.numeric == 102) )
            cond2 = 1 ;
    }

    return (cond1 & cond2) ;
}

/*
** convert_simple2complex()
** 
*/
void convert_simple2complex(ZebraHandle zh, Z_RPNStructure *rpnstruct )
{
    Z_Complex *complex = NULL ;                                         
    Z_Operand *operand = rpnstruct->u.simple ;                          

    if ( check_operand_attrs( operand ) )
    {
        complex = expand_query(zh, operand ) ;

        if ( complex )
        {
            /*
            ** Everything is complete so replace the original
            ** operand with the newly built complex structure
            ** This is it ... no going back!!
            */
            rpnstruct->which     = Z_RPNStructure_complex ;
            rpnstruct->u.complex = complex ;
        }
    }                                                                       
}

/*
** walk_complex_query()
** recursively traverse the tree expanding any simple queries we find
*/
void walk_complex_query(ZebraHandle zh, Z_RPNStructure *rpnstruct )
{
    if ( rpnstruct->which == Z_RPNStructure_simple )
    {
        convert_simple2complex(zh, rpnstruct ) ;
    }
    else
    {
        walk_complex_query(zh, rpnstruct->u.complex->s1 ) ;
        walk_complex_query(zh, rpnstruct->u.complex->s2 ) ;
    }
}

void zebra_livcode_transform(ZebraHandle zh, Z_RPNQuery *query)
{
    /*
    ** Got a search request,
    ** 1. if it is a simple query, see if it suitable for expansion
    **    i.e. the attributes are of the form ...
    **    (type==1 && value==1016) && (type==2 && value==102)
    ** or
    ** 2. if it is complex, traverse the complex query tree and expand
    **    any simples simples as above
    */
#if LIV_CODE
    Z_RPNStructure *rpnstruct = query->RPNStructure ;
    
    if ( rpnstruct->which == Z_RPNStructure_simple )
    {
        convert_simple2complex(zh, rpnstruct ) ;
    }
    else if ( rpnstruct->which == Z_RPNStructure_complex )
    {
        walk_complex_query(zh, rpnstruct ) ;
    }
#endif
}


struct rank_class_info {
    int dummy;
};

struct rank_term_info {
    int local_occur;
    int global_occur;
    int global_inv;
    int rank_flag;
};

struct rank_set_info {
    int last_pos;
    int no_entries;
    int no_rank_entries;
    struct rank_term_info *entries;
};

static int log2_int (unsigned g)
{
    int n = 0;
    while ((g = g>>1))
	n++;
    return n;
}

/*
 * create: Creates/Initialises this rank handler. This routine is 
 *  called exactly once. The routine returns the class_handle.
 */
static void *create (ZebraHandle zh)
{
    struct rank_class_info *ci = (struct rank_class_info *)
	xmalloc (sizeof(*ci));

    logf (LOG_DEBUG, "livrank create");

    read_zrank_file(zh) ;

    return ci;
}

/*
 * destroy: Destroys this rank handler. This routine is called
 *  when the handler is no longer needed - i.e. when the server
 *  dies. The class_handle was previously returned by create.
 */
static void destroy (struct zebra_register *reg, void *class_handle)
{
    struct rank_class_info *ci = (struct rank_class_info *) class_handle;

    logf (LOG_DEBUG, "livrank destroy");
    xfree (ci);
}


/*
 * begin: Prepares beginning of "real" ranking. Called once for
 *  each result set. The returned handle is a "set handle" and
 *  will be used in each of the handlers below.
 */
static void *begin (struct zebra_register *reg, void *class_handle, RSET rset)
{
    struct rank_set_info *si = (struct rank_set_info *) xmalloc (sizeof(*si));
    int i;

    logf (LOG_DEBUG, "livrank begin");
    si->no_entries = rset->no_rset_terms;
    si->no_rank_entries = 0;
    si->entries = (struct rank_term_info *)
	xmalloc (sizeof(*si->entries)*si->no_entries);
    for (i = 0; i < si->no_entries; i++)
    {
        const char *flags = rset->rset_terms[i]->flags;
	int g = rset->rset_terms[i]->nn;
        const char *cp = strstr(flags, ",u=");

        si->entries[i].rank_flag = 1;
        if (cp)
        {
            char *t = search_for_rankstr(atoi(cp+3));
            if (t)
                si->entries[i].rank_flag = search_for_score(t) ;
        }
        if ( si->entries[i].rank_flag )
            (si->no_rank_entries)++;

	si->entries[i].local_occur = 0;
	si->entries[i].global_occur = g;
	si->entries[i].global_inv = 32 - log2_int (g);
	logf (LOG_DEBUG, "-------- %d ------", 32 - log2_int (g));
    }
    return si;
}

/*
 * end: Terminates ranking process. Called after a result set
 *  has been ranked.
 */
static void end (struct zebra_register *reg, void *set_handle)
{
    struct rank_set_info *si = (struct rank_set_info *) set_handle;
    logf (LOG_DEBUG, "livrank end");
    xfree (si->entries);
    xfree (si);
}

/*
 * add: Called for each word occurence in a result set. This routine
 *  should be as fast as possible. This routine should "incrementally"
 *  update the score.
 */
static void add (void *set_handle, int seqno, int term_index)
{
    struct rank_set_info *si = (struct rank_set_info *) set_handle;
    logf (LOG_DEBUG, "rank-1 add seqno=%d term_index=%d", seqno, term_index);
    si->last_pos = seqno;
    si->entries[term_index].local_occur++;
}

/*
 * calc: Called for each document in a result. This handler should 
 *  produce a score based on previous call(s) to the add handler. The
 *  score should be between 0 and 1000. If score cannot be obtained
 *  -1 should be returned.
 */
static int calc (void *set_handle, zint sysno)
{
    int i, lo, divisor, score = 0;
    struct rank_set_info *si = (struct rank_set_info *) set_handle;

    logf (LOG_DEBUG, "livrank calc sysno=" ZINT_FORMAT, sysno);

    if (!si->no_rank_entries)
	return -1;
    for (i = 0; i < si->no_entries; i++)
    {
        score += si->entries[i].local_occur * si->entries[i].rank_flag ;
    }
    for (i = 0; i < si->no_entries; i++)
	if (si->entries[i].rank_flag && (lo = si->entries[i].local_occur))
	    score += (8+log2_int (lo)) * si->entries[i].global_inv;
    score *= 34;
    divisor = si->no_rank_entries * (8+log2_int (si->last_pos/si->no_entries));
    score = score / divisor;
    if (score > 1000)
	score = 1000;
    for (i = 0; i < si->no_entries; i++)
	si->entries[i].local_occur = 0;
    return score;
}

/*
 * Pseudo-meta code with sequence of calls as they occur in a
 * server. Handlers are prefixed by --:
 *
 *     server init
 *     -- create
 *     foreach search
 *        rank result set
 *        -- begin
 *        foreach record
 *           foreach word
 *              -- add
 *           -- calc
 *        -- end
 *     -- destroy
 *     server close
 */

static struct rank_control rank_control = {
    "livrank",
    create,
    destroy,
    begin,
    end,
    calc,
    add,
};
 
struct rank_control *rankliv_class = &rank_control;

/*
 * Copyright (C) 1994-1995, Index Data I/S 
 * All rights reserved.
 * Sebastian Hammer, Adam Dickmeiss
 *
 * $Log: zlogs.c,v $
 * Revision 1.1  1995-11-16 17:00:55  adam
 * Better logging of rpn query.
 *
 */
#include <stdio.h>
#include <assert.h>


#include "zserver.h"

static char *attrStr (int type, int value, enum oid_value ast)
{
    static char str[80];

    switch (ast)
    {
    case VAL_BIB1:
    case VAL_EXP1:
    case VAL_GILS:
        switch (type)
        {
        case 1:
            sprintf (str, "use=%d", value);
            return str;
        case 2:
            sprintf (str, "relation=%d", value);
            return str;
        case 3:
            sprintf (str, "position=%d", value);
            return str;
        case 4:
            sprintf (str, "structure=%d", value);
            return str;
        case 5:
            sprintf (str, "truncation=%d", value);
            return str;
        case 6:
            sprintf (str, "completeness=%d", value);
            return str;
        }
        break;
    default:
        break;
    }
    sprintf (str, "%d=%d", type, value);
    return str;
}

/*
 * zlog_attributes: print attributes of term
 */
static void zlog_attributes (Z_AttributesPlusTerm *t, int level,
                             enum oid_value ast)
{
    int of, i;
    for (of = 0; of < t->num_attributes; of++)
    {
        Z_AttributeElement *element;
        element = t->attributeList[of];

        switch (element->which) 
        {
        case Z_AttributeValue_numeric:
            logf (LOG_LOG, "%*.s %s", level, "",
                  attrStr (*element->attributeType, *element->value.numeric, ast));
            break;
        case Z_AttributeValue_complex:
            logf (LOG_LOG, "%*.s attributeType=%d complex", level, "",
                  *element->attributeType);
            for (i = 0; i<element->value.complex->num_list; i++)
            {
                if (element->value.complex->list[i]->which ==
                    Z_StringOrNumeric_string)
                    logf (LOG_LOG, "%*.s  string: '%s'", level, "",
                          element->value.complex->list[i]->u.string);
                else if (element->value.complex->list[i]->which ==
                         Z_StringOrNumeric_numeric)
                    logf (LOG_LOG, "%*.s  numeric: '%d'", level, "",
                          *element->value.complex->list[i]->u.numeric);
            }
            break;
        default:
            logf (LOG_LOG, "%.*s attribute unknown", level, "");
        }
    }
}

static void zlog_structure (Z_RPNStructure *zs, int level, enum oid_value ast)
{
    if (zs->which == Z_RPNStructure_complex)
    {
        switch (zs->u.complex->operator->which)
        {
        case Z_Operator_and:
            logf (LOG_LOG, "%*.s and", level, "");
            break;
        case Z_Operator_or:
            logf (LOG_LOG, "%*.s or", level, "");
            break;
        case Z_Operator_and_not:
            logf (LOG_LOG, "%*.s and", level, "");
            break;
        default:
            logf (LOG_LOG, "%*.s unknown complex", level, "");
            return;
        }
        zlog_structure (zs->u.complex->s1, level+2, ast);
        zlog_structure (zs->u.complex->s2, level+2, ast);
    }
    else if (zs->which == Z_RPNStructure_simple)
    {
        if (zs->u.simple->which == Z_Operand_APT)
        {
            Z_AttributesPlusTerm *zapt = zs->u.simple->u.attributesPlusTerm;

            if (zapt->term->which == Z_Term_general) 
            {
                logf (LOG_LOG, "%*.s term '%.*s' (general)", level, "",
                      zapt->term->u.general->len, zapt->term->u.general->buf);
            }
            else
            {
                logf (LOG_LOG, "%*.s term (not general)", level, "");
            }
            zlog_attributes (zapt, level+2, ast);
        }
        else if (zs->u.simple->which == Z_Operand_resultSetId)
        {
            logf (LOG_LOG, "%*.s set '%s'", level, "",
                  zs->u.simple->u.resultSetId);
        }
        else
            logf (LOG_LOG, "%*.s unknown simple structure", level, "");
    }
    else
        logf (LOG_LOG, "%*.s unknown structure", level, "");
}

void zlog_rpn (Z_RPNQuery *rpn)
{
    oident *attrset = oid_getentbyoid (rpn->attributeSetId);
    enum oid_value ast;

    ast = attrset->value;
    logf (LOG_LOG, "RPN query. Type: %s", attrset->desc);
    zlog_structure (rpn->RPNStructure, 0, ast);
}

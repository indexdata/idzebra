/* This file is part of the Zebra server.
   Copyright (C) 1994-2011 Index Data

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

#if HAVE_CONFIG_H
#include <config.h>
#endif
#include <stdio.h>
#include <assert.h>
#include <stdlib.h>

#include <yaz/log.h>
#include <idzebra/data1.h>


void data1_remove_node (data1_handle dh, data1_node *n)
{
    fprintf (stdout, "REMOVE tag %s \n", n->u.tag.tag);
    /* n is first childen */
    if(n->parent->child == n){
        n->parent->child = n->next;

        /* n is the only child */
        if(! n->next){
            n->parent->last_child = 0;                
        }
    } 
    /* n is one of the following childrens */
    else {
        data1_node * before;
        
        /* need to find sibling before me */
        before = n->parent->child;
        while (before->next != n)
            before = before->next;
        
        before->next = n->next;
        
        /* n is last child of many */
        if ( n->parent->last_child == n){
            n->parent->last_child = before;
        }
    }
    /* break pointers to root, parent and following siblings */
    n->parent = 0;
    n->root = 0;
    n->next = 0;
}

void data1_remove_idzebra_subtree (data1_handle dh, data1_node *n)
{
    switch (n->which)
        {
            /*
        case DATA1N_root:
            break;
            */
        case DATA1N_tag:
            if (!strcmp(n->u.tag.tag, "idzebra")){
                if (n->u.tag.attributes){
                    data1_xattr *xattr = n->u.tag.attributes;
                    
                    for (; xattr; xattr = xattr->next){
                        if (!strcmp(xattr->name, "xmlns") 
                            & !strcmp(xattr->value, 
                                      "http://www.indexdata.dk/zebra/"))
                            data1_remove_node (dh, n);
                    }
                }
            }
                
            break;
            /*
        case DATA1N_data:
            break;
        case DATA1N_comment:
            break;
        case DATA1N_preprocess:
            break;
        case DATA1N_variant:
        break;
            */
        default:
            break;
        }
    if (n->child)
        data1_remove_idzebra_subtree (dh, n->child);
    if (n->next)
        data1_remove_idzebra_subtree (dh, n->next);
    /*
    else
        {
            if (n->parent && n->parent->last_child != n)
                fprintf(out, "%*sWARNING: last_child=%p != %p\n", level, "",
                        n->parent->last_child, n);
        }
    */
    
}



/*
 * Local variables:
 * c-basic-offset: 4
 * c-file-style: "Stroustrup"
 * indent-tabs-mode: nil
 * End:
 * vim: shiftwidth=4 tabstop=8 expandtab
 */


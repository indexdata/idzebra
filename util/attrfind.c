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

#include <assert.h>

#include <attrfind.h>

void attr_init_APT(AttrType *src, Z_AttributesPlusTerm *zapt, int type)
{
    src->attributeList = zapt->attributes->attributes;
    src->num_attributes = zapt->attributes->num_attributes;
    src->type = type;
    src->major = 0;
    src->minor = 0;
}

void attr_init_AttrList(AttrType *src, Z_AttributeList *list, int type)
{
    src->attributeList = list->attributes;
    src->num_attributes = list->num_attributes;
    src->type = type;
    src->major = 0;
    src->minor = 0;
}

int attr_find_ex(AttrType *src, const Odr_oid **attribute_set_oid,
		 const char **string_value)
{
    int num_attributes;

    num_attributes = src->num_attributes;
    while (src->major < num_attributes)
    {
        Z_AttributeElement *element;

        element = src->attributeList[src->major];
        if (src->type == *element->attributeType)
        {
            switch (element->which) 
            {
            case Z_AttributeValue_numeric:
                ++(src->major);
                if (element->attributeSet && attribute_set_oid)
                    *attribute_set_oid = element->attributeSet;
                return *element->value.numeric;
                break;
            case Z_AttributeValue_complex:
                if (src->minor >= element->value.complex->num_list)
                    break;
                if (element->attributeSet && attribute_set_oid)
                    *attribute_set_oid = element->attributeSet;
                if (element->value.complex->list[src->minor]->which ==  
                    Z_StringOrNumeric_numeric)
                {
                    ++(src->minor);
                    return
                        *element->value.complex->list[src->minor-1]->u.numeric;
                }
                else if (element->value.complex->list[src->minor]->which ==  
                         Z_StringOrNumeric_string)
                {
                    if (!string_value)
                        break;
                    ++(src->minor);
                    *string_value = 
                        element->value.complex->list[src->minor-1]->u.string;
                    return -2;
                }
                else
                    break;
            default:
                assert(0);
            }
        }
        ++(src->major);
    }
    return -1;
}

int attr_find(AttrType *src, const Odr_oid **attribute_set_id)
{
    return attr_find_ex(src, attribute_set_id, 0);
}


/*
 * Local variables:
 * c-basic-offset: 4
 * c-file-style: "Stroustrup"
 * indent-tabs-mode: nil
 * End:
 * vim: shiftwidth=4 tabstop=8 expandtab
 */


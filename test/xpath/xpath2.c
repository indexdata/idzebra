/* $Id: xpath2.c,v 1.4 2005-09-13 11:51:11 adam Exp $
   Copyright (C) 1995-2005
   Index Data ApS

This file is part of the Zebra server.

Zebra is free software; you can redistribute it and/or modify it under
the terms of the GNU General Public License as published by the Free
Software Foundation; either version 2, or (at your option) any later
version.

Zebra is distributed in the hope that it will be useful, but WITHOUT ANY
WARRANTY; without even the implied warranty of MERCHANTABILITY or
FITNESS FOR A PARTICULAR PURPOSE.  See the GNU General Public License
for more details.

You should have received a copy of the GNU General Public License
along with Zebra; see the file LICENSE.zebra.  If not, write to the
Free Software Foundation, 59 Temple Place - Suite 330, Boston, MA
02111-1307, USA.
*/

#include "../api/testlib.h"

/** xpath2.c - index a a bit more complex sgml record and search in it */

const char *myrec[] = {
    "<Zthes> \n"
    " <termId>10</termId> \n"
    " <termName>Sauropoda</termName> \n"
    " <termType>PT</termType> \n"
    " <relation> \n"
    "  <relationType>BT</relationType> \n"
    "  <termId>5</termId> \n"
    "  <termName>Brontosauria</termName> \n"
    "  <termType>PT</termType> \n"
    " </relation> \n"
    " <relation> \n"
    "  <relationType>NT</relationType> \n"
    "  <termId>11</termId> \n"
    "  <termName>Eusauropoda</termName> \n"
    "  <termType>PT</termType> \n"
    " </relation> \n"
    "</Zthes> \n",

    "<Zthes> \n"
    " <termId>5</termId> \n"
    " <termName>Brontosauria</termName> \n"
    " <termType>PT</termType> \n"
    " <relation> \n"
    "  <relationType>BT</relationType> \n"
    "  <termId>4</termId> \n"
    "  <termName>Sauropodomorpha</termName> \n"
    "  <termType>PT</termType> \n"
    " </relation> \n"
    " <relation> \n"
    "  <relationType>NT</relationType> \n"
    "  <termId>6</termId> \n"
    "  <termName>Plateosauria</termName> \n"
    "  <termType>PT</termType> \n"
    " </relation> \n"
    " <relation> \n"
    "  <relationType>NT</relationType> \n"
    "  <termId>10</termId> \n"
    "  <termName>Sauropoda</termName> \n"
    "  <termType>PT</termType> \n"
    " </relation> \n"
    "</Zthes> \n",
    0};


int main(int argc, char **argv)
{
    ZebraService zs = start_up(0, argc, argv);
    ZebraHandle zh = zebra_open(zs, 0);
    init_data(zh, myrec);

    do_query(__LINE__,zh, "@attr 1=/Zthes/termName Sauropoda", 1);
    do_query(__LINE__,zh, "@attr 1=/Zthes/relation/termName Sauropoda",1);

    return close_down(zh, zs, 0);
}

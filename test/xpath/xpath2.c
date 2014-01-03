/* This file is part of the Zebra server.
   Copyright (C) Index Data

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


static void tst(int argc, char **argv)
{
    ZebraService zs = tl_start_up(0, argc, argv);
    ZebraHandle zh = zebra_open(zs, 0);
    YAZ_CHECK(tl_init_data(zh, myrec));

    YAZ_CHECK(tl_query(zh, "@attr 1=/Zthes/termName Sauropoda", 1));
    YAZ_CHECK(tl_query(zh, "@attr 1=/Zthes/relation/termName Sauropoda", 1));

    YAZ_CHECK(tl_close_down(zh, zs));
}

TL_MAIN

/*
 * Local variables:
 * c-basic-offset: 4
 * c-file-style: "Stroustrup"
 * indent-tabs-mode: nil
 * End:
 * vim: shiftwidth=4 tabstop=8 expandtab
 */


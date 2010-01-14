/* This file is part of the Zebra server.
   Copyright (C) 1994-2010 Index Data

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

#include <yaz/test.h>
#include <dfa.h>

int test_parse(struct DFA *dfa, const char *pattern)
{
    int i;
    const char *cp = pattern;
    i = dfa_parse(dfa, &cp);
    return i;
}
    
static void tst(int argc, char **argv)
{
    struct DFA *dfa = dfa_init();
    YAZ_CHECK(dfa);

    YAZ_CHECK_EQ(test_parse(dfa, ""), 1);
    YAZ_CHECK_EQ(test_parse(dfa, "a"), 0);
    YAZ_CHECK_EQ(test_parse(dfa, "ab"), 0);
    YAZ_CHECK_EQ(test_parse(dfa, "(a)"), 0);
    YAZ_CHECK_EQ(test_parse(dfa, "(a"), 1);
    YAZ_CHECK_EQ(test_parse(dfa, "a)"), 3);
    YAZ_CHECK_EQ(test_parse(dfa, "a\001)"), 0);
    YAZ_CHECK_EQ(test_parse(dfa, "a\\x01"), 0);


    YAZ_CHECK_EQ(test_parse(dfa, "(\x01\x02)(CEO3EQC/\\x01\\x0C\\x01\\x0C)"), 0);
    dfa_delete(&dfa);
    YAZ_CHECK(!dfa);
}

int main(int argc, char **argv)
{
    YAZ_CHECK_INIT(argc, argv);
    YAZ_CHECK_LOG();
    tst(argc, argv);
    YAZ_CHECK_TERM;
}

/*
 * Local variables:
 * c-basic-offset: 4
 * c-file-style: "Stroustrup"
 * indent-tabs-mode: nil
 * End:
 * vim: shiftwidth=4 tabstop=8 expandtab
 */


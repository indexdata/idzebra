/*
 * Copyright (C) 1994, Index Data I/S 
 * All rights reserved.
 * Sebastian Hammer, Adam Dickmeiss
 *
 * $Log: lexer.h,v $
 * Revision 1.1  1994-09-26 10:16:55  adam
 * First version of dfa module in alex. This version uses yacc to parse
 * regular expressions. This should be hand-made instead.
 *
 */


int        read_file      ( const char *, DFA ** );
void       error          ( const char *, ... );

extern int ccluse;


#!/usr/bin/perl

BEGIN {
    push (@INC,'../blib/lib','../blib/arch','../blib/lib');
}

use pod;

$res =pod->test($ARGV[0],
	  (tabPath=>'../blib/lib:../blib/arch:../blib/lib:.:../../tab:../../../yaz/tab'));

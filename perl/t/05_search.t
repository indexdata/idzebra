#!perl
# =============================================================================
# $Id: 05_search.t,v 1.1 2003-03-03 00:44:39 pop Exp $
#
# Perl API header
# =============================================================================
BEGIN {
    if ($ENV{PERL_CORE}) {
        chdir 't' if -d 't';
    }
    push (@INC,'demo','blib/lib','blib/arch');
}

use strict;
use warnings;

use Test::More tests => 7;

# ----------------------------------------------------------------------------
# Session opening and closing
BEGIN {
    use IDZebra;
    IDZebra::logFile("test.log");
    use_ok('IDZebra::Session'); 
    use_ok('pod');
}


# ----------------------------------------------------------------------------
# Session opening and closing
my $sess = IDZebra::Session->open(configFile => 'demo/zebra.cfg',
				  groupName => 'demo2');
isa_ok($sess,"IDZebra::Session");

# ----------------------------------------------------------------------------
# search
our $filecount = 6;

my ($hits, $expected);

# Search 1 databases
my $rs1 = $sess->search(cqlmap    => 'demo/cql.map',
			cql       => 'IDZebra',
			databases => [qw(demo1)]);

$expected = $filecount;
$hits = $rs1->count;
ok(($hits == $expected), "CQL search - found $hits/$expected records");


$sess->databases('demo1', 'demo2');
my @dblist = $sess->databases;
ok(($#dblist == 1), "Select multiple databases"); 


# Search 2 databases
my $rs2 = $sess->search(cqlmap    => 'demo/cql.map',
			cql       => 'IDZebra');
$expected = $filecount * 2;
$hits = $rs2->count;
ok(($hits == $expected), "CQL search - found $hits/$expected records");

# RPN search;
my $rs3 = $sess->search(cqlmap    => 'demo/cql.map',
			pqf       => '@attr 1=4 IDZebra');
$expected = $filecount * 2;
$hits = $rs2->count;
ok(($hits == $expected), "RPN search - found $hits/$expected records");


# More specific search


# ----------------------------------------------------------------------------
# Close session

$sess->close;

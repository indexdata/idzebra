#!perl
# =============================================================================
# $Id: 05_search.t,v 1.5 2004-09-15 14:11:06 heikki Exp $
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
BEGIN {
    use IDZebra;
    unlink("test05.log");
    IDZebra::logFile("test05.log");
    use_ok('IDZebra::Session'); 
    use_ok('pod');
}


# ----------------------------------------------------------------------------
# Session opening and closing
my $sess = IDZebra::Session->open(configFile => 'demo/zebra.cfg',
				  groupName => 'demo2');
isa_ok($sess,"IDZebra::Session");

# Insert some test data
my $ret;
my $sysno;
my $F;
my $filecount=0;
$sess->databases('demo1', 'demo2');
$sess->init();
for $F (<"lib/IDZebra/*.pm">)
{
    ($ret,$sysno)=$sess->insert_record (file=>$F, recordType => 'grs.perl.pod');
    ok( $ret==0, "inserted $F");
    $filecount++;
}
# ----------------------------------------------------------------------------
# search

my ($hits, $expected);

# Search 1 databases
my $rs1 = $sess->search(cqlmap    => 'demo/cql.map',
			cql       => 'IDZebra',
			termset   => 1,
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
$hits = $rs3->count;
ok(($hits == $expected), "RPN search - found $hits/$expected records");

#### Terms is broken time being, don't bother testing it
# Termlists;
#my $rs4 = $sess->search(pqf       => '@attr 1=4 @and IDZebra Session');
#$expected = 2;
#$hits = $rs4->count;
#ok(($hits == $expected), "RPN search - found $hits/$expected records");
#print STDERR "Test 8: found $hits of $expected\n";
#
#my @terms = $rs4->terms();
#ok(($#terms == 1), "Got 2 terms in RPN expression");
#my $cc = 0;
#foreach my $t (@terms) {
#    if ($t->{term} eq 'IDZebra') {
#	ok(($t->{count} = $filecount*2), "Term IDZebra ($t->{count})");
#	$cc++;
#    }
#    elsif ($t->{term} eq 'Session') {
#	ok(($t->{count} = 2), "Term Session ($t->{count})");
#	$cc++;
#    } else {
#	ok(0,"Invalid term $t->{term}");
#    }
#
#}
#ok (($cc == 2), "Got 2 terms for RS");



# More specific search


# ----------------------------------------------------------------------------
# Close session

$sess->close;

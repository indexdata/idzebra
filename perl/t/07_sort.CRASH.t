#!perl
# =============================================================================
# $Id: 07_sort.CRASH.t,v 1.1 2004-09-15 14:11:06 heikki Exp $
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

use Test::More skip_all =>"sort into a new rset crashes due to rset_dup bug";
#use Test::More tests => 13;

# ----------------------------------------------------------------------------
# Session opening and closing
BEGIN {
    use IDZebra;
    unlink("test07.log");
    IDZebra::logFile("test07.log");
#  IDZebra::logLevel(0x4F);
#  IDZebra::logLevel(15);
    use_ok('IDZebra::Session'); 
    use_ok('pod');
}


# ----------------------------------------------------------------------------
# Session opening and closing
my $sess = IDZebra::Session->open(configFile => 'demo/zebra.cfg',
				  groupName => 'demo2');
# ----------------------------------------------------------------------------
# Insert some test data
my $ret;
my $sysno;
my $F;
my $filecount=0;
$sess->init;
$sess->begin_trans;
$sess->databases('demo1', 'demo2');
$ret=$sess->end_trans;

$sess->begin_trans;
for $F (<lib/IDZebra/*.pm>)
{
    ($ret,$sysno)=$sess->insert_record (file=>$F, recordType => 'grs.perl.pod');
    ok( $ret==0, "inserted $F");
    #print STDERR "Inserted $F ok. ret=$ret sys=$sysno\n";
    $filecount++;
}
$ret=$sess->end_trans;
ok($filecount>0,"Inserted files");
is($ret->{inserted},$filecount, "Inserted all");


# -----------------------------------------------------------------------------
# Search 1 database, retrieve records, sort "titles" locally (dangerous!)

my $rs1 = $sess->search(cqlmap    => 'demo/cql.map',
			cql       => 'IDZebra',
			databases => [qw(demo1)]);
$rs1 = $rs1->sort('1=4 id');

# -----------------------------------------------------------------------------
# Sort descending, new rs

my $rs2 = $rs1->sort('1=4 id');

isa_ok ($rs2, 'IDZebra::Resultset');

# ----------------------------------------------------------------------------
# Close session
$sess->close;


#!perl
# =============================================================================
# $Id: 07_sort.t,v 1.4 2004-09-16 15:07:55 heikki Exp $
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

use Test::More tests => 32;

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

my (@unsorted, @sorted, @sortedi);

my $wasError = 0;
my $sortError = 0;
foreach my $rec ($rs1->records()) {
    if ($rec->{errCode}) {
	$wasError++; 
    }
    my ($title) = ($rec->buf =~ /\n\s*package\s+([a-zA-Z0-9:]+)\s*\;\s*\n/);
    push (@unsorted, $title);
}
ok (($wasError == 0), "retrieval");

@sorted = sort (@unsorted);
no warnings;
@sortedi = sort ({my $a1=$a; $a1 =~ y/[A-Z]/[a-z]/; 
		  my $b1=$b; $b1 =~ y/[A-Z]/[a-z]/; 
		  ($a1 cmp $b1);} @unsorted);
use warnings;

# -----------------------------------------------------------------------------
# Sort rs itself ascending

isa_ok ($rs1, 'IDZebra::Resultset');

$rs1->sort('1=4 ia');

isa_ok ($rs1, 'IDZebra::Resultset');

$wasError = 0;
$sortError = 0;
foreach my $rec ($rs1->records()) {
    if ($rec->{errCode}) { $wasError++; }
    my ($title) = ($rec->buf =~ /\n\s*package\s+([a-zA-Z0-9:]+)\s*\;\s*\n/);
    if ($sortedi[$rec->position - 1] ne $title) { $sortError++; }
}

ok (($wasError == 0), "retrieval");
ok (($sortError == 0), "sorting ascending");

# -----------------------------------------------------------------------------
# Sort descending, new rs
#TODO: {
#  todo_skip "Sort into different rset crashes", 3;

my $rs2 = $rs1->sort('1=4 id');

isa_ok ($rs2, 'IDZebra::Resultset');

$wasError = 0;
$sortError = 0;
foreach my $rec ($rs2->records()) {
    if ($rec->{errCode}) { $wasError++; }
    my ($title) = ($rec->buf =~ /\n\s*package\s+([a-zA-Z0-9:]+)\s*\;\s*\n/);
    if ($sorted[$rs2->count - $rec->position] ne $title) { $sortError++; }
    is ($title, $sorted[$rs2->count - $rec->position], "sort order $title");
}


is ($wasError,0 , "retrieval");
is ($sortError, 0, "sorting descending");

# } # end of SKIP

# -----------------------------------------------------------------------------
# Search + sort ascending
my $rs3 = $sess->search(cql       => 'IDZebra',
			databases => [qw(demo1)],
			sort      => '1=4 ia');
isa_ok ($rs3, 'IDZebra::Resultset');

$wasError = 0;
$sortError = 0;
foreach my $rec ($rs3->records()) {
    if ($rec->{errCode}) { $wasError++; }
    my ($title) = ($rec->buf =~ /\n\s*package\s+([a-zA-Z0-9:]+)\s*\;\s*\n/);
    if ($sortedi[$rec->position - 1] ne $title) { $sortError++; }
}

ok (($wasError == 0), "saerch+sort, retrieval");
ok (($sortError == 0), "search+sort descending");

# ----------------------------------------------------------------------------
# Bad sort

my $rs4;
$rs4 = $rs3->sort("ostrich");
ok (($rs4->errCode != 0),"Wrong sort: ".$rs4->errCode."(".$rs4->errString.")");
# ----------------------------------------------------------------------------
# Close session
$sess->close;


#!perl
# =============================================================================
# $Id: 06_retrieval.t,v 1.6 2004-09-15 14:11:06 heikki Exp $
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

use Test::More tests => 30;

# ----------------------------------------------------------------------------
# Session opening and closing
BEGIN {
    use IDZebra;
    unlink("test06.log");
    IDZebra::logFile("test06.log");
    use_ok('IDZebra::Session'); 
    use_ok('pod');
}


# ----------------------------------------------------------------------------
# Session opening and closing
my $sess = IDZebra::Session->open(configFile => 'demo/zebra.cfg',
				  groupName => 'demo2');
ok($sess,"session");

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

# search

my ($hits, $expected);

# Search 1 database
my $rs1 = $sess->search(cqlmap    => 'demo/cql.map',
			cql       => 'IDZebra',
			databases => [qw(demo1)]);

$expected = $filecount;
$hits = $rs1->count;
is($hits, $expected, "CQL search ");

foreach my $rec ($rs1->records(from =>1,
			      to   =>5)) {
    isa_ok($rec,'IDZebra::RetrievalRecord');
}

my (@recs) = $rs1->records(from=>1,to=>1);

ok ($#recs == 0, "Fetched 1 record");

my $rec1 = shift(@recs);

isa_ok($rec1,'IDZebra::RetrievalRecord');
ok (($rec1->{errCode} == 0), "err: $rec1->{errCode}");
ok (($rec1->{errString} eq ""), "errString: $rec1->{errString}");
ok (($rec1->{position} == 1), "position: $rec1->{position}");
ok (($rec1->{base} eq 'demo1'), "base: $rec1->{base}");
ok (($rec1->{sysno}), "sysno: $rec1->{sysno}");
ok (($rec1->{score}), "score: $rec1->{score}");
ok (($rec1->{format} eq 'SUTRS'), "format: $rec1->{format}");
ok ((length($rec1->{buf}) > 0), "buf: ". length($rec1->{buf})." bytes");

# ----------------------------------------------------------------------------
# Close session, check for rs availability

$sess=undef;

eval { my ($rec2) = $rs1->records(from=>1,to=>1); };

ok (($@ ne ""), "Resultset is invalidated with session");
# ----------------------------------------------------------------------------
# Code from doc...
#  foreach my $rec ($rs1->records()) {
#      print STDERR "REC:$rec\n";
#      unless ($rec->errCode) {
#         printf  ("Pos:%d, Base: %s, sysno: %d, score %d format: %s\n%s\n\n",
#             $rec->position,
#             $rec->base,
#             $rec->sysno,
#             $rec->score,
#             $rec->format,
#             $rec->buf
#         );
#      }
#  }



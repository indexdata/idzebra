#!perl
# =============================================================================
# $Id: 06_retrieval.t,v 1.5 2004-07-28 08:15:47 adam Exp $
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

use Test::More tests => 19;

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
# ----------------------------------------------------------------------------
# search
our $filecount = 8;

my ($hits, $expected);

# Search 1 database
my $rs1 = $sess->search(cqlmap    => 'demo/cql.map',
			cql       => 'IDZebra',
			databases => [qw(demo1)]);

$expected = $filecount;
$hits = $rs1->count;
ok(($hits == $expected), "CQL search - found $hits/$expected records");

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



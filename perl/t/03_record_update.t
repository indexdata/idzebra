#!perl
# =============================================================================
# $Id: 03_record_update.t,v 1.2 2003-03-05 13:55:22 pop Exp $
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
    use_ok('IDZebra');
    IDZebra::logFile("test.log");
    use_ok('IDZebra::Session'); 
    use_ok('pod');
}


# ----------------------------------------------------------------------------
# Session opening and closing
my $sess = IDZebra::Session->open(configFile => 'demo/zebra.cfg',
				  groupName => 'demo2',
				  shadow    => 1);
isa_ok($sess,"IDZebra::Session");

# ----------------------------------------------------------------------------
# per record update
my $rec1=`cat lib/IDZebra/Data1.pm`;
my $rec2=`cat lib/IDZebra/Filter.pm`;

my ($sysno, $stat);

$sess->begin_trans;
$sysno = $sess->update_record(data       => $rec1,
			      recordType => 'grs.perl.pod',
			      groupName  => "demo1",
			      );
$stat = $sess->end_trans;
ok(($stat->{updated} == 1), "Updated 1 records");

$sess->begin_trans;
$sysno = $sess->delete_record(data       => $rec2,
			      recordType => 'grs.perl.pod',
			      groupName  => "demo1",
			      );
$stat = $sess->end_trans;
ok(($stat->{deleted} == 1), "Deleted 1 records");

$sess->begin_trans;
$sysno = $sess->update_record(data       => $rec2,
			       recordType => 'grs.perl.pod',
			       groupName  => "demo1",
			       );
$stat = $sess->end_trans;
ok(($stat->{inserted} == 1), "Inserted 1 records");




# ----------------------------------------------------------------------------
# Close session

$sess->commit;
$sess->close;

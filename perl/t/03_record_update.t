#!perl
# =============================================================================
# $Id: 03_record_update.t,v 1.4 2003-04-15 20:55:14 pop Exp $
#
# Perl API header
# =============================================================================
BEGIN {
    if ($ENV{PERL_CORE}) {
        chdir 't' if -d 't';
    }
    unshift (@INC,'demo','blib/lib','blib/arch');
}

use strict;
use warnings;

use Test::More tests => 17;

# ----------------------------------------------------------------------------
# Session opening and closing
BEGIN {
    use_ok('IDZebra');
#    IDZebra::logFile("test.log");
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
my $rec3=`cat lib/IDZebra/Session.pm`;

# IDZebra::logLevel(15);

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
$sysno = $sess->insert_record(data       => $rec2,
			      recordType => 'grs.perl.pod',
			      groupName  => "demo1",
			      );
$stat = $sess->end_trans;
ok(($stat->{inserted} == 1), "Inserted 1 records");
ok(($sysno > 0),"Inserted record got valid sysno");

$sess->begin_trans;
$sysno = $sess->insert_record(data       => $rec2,
			      recordType => 'grs.perl.pod',
			      groupName  => "demo1",
			      );
$stat = $sess->end_trans;
ok(($stat->{inserted} == 0), "Inserted 0 records");
ok(($stat->{updated} == 0), "Updated 0 records");
ok(($sysno < 0),"Inserted record got invalid sysno");


$sess->begin_trans;
$sysno = $sess->update_record(data       => $rec2,
			      recordType => 'grs.perl.pod',
			      groupName  => "demo1",
			      );

$sysno = $sess->update_record(data       => $rec2,
			      recordType => 'grs.perl.pod',
			      groupName  => "demo1",
			      );

$stat = $sess->end_trans;
ok(($stat->{inserted} == 0), "Inserted 0 records");
ok(($stat->{updated} == 1), "Updated $stat->{updated} records");
ok(($sysno > 0),"Inserted got valid sysno");

$sess->begin_trans;
$sysno = $sess->delete_record(data       => $rec3,
			      recordType => 'grs.perl.pod',
			      groupName  => "demo1",
			      );
$stat = $sess->end_trans;


$sess->begin_trans;
$sysno = $sess->update_record(data       => $rec2,
			      recordType => 'grs.perl.pod',
			      groupName  => "demo1",
			      );

foreach my $i (1..100) {
    $sysno = $sess->update_record(data       => $rec2,
				  recordType => 'grs.perl.pod',
				  groupName  => "demo1",
				  force      => 1,
				  );
}
foreach my $i (1..10) {
    $sysno = $sess->update_record(data       => $rec3,
				  recordType => 'grs.perl.pod',
				  groupName  => "demo1",
				  force      => 1,
				  );
}
foreach my $i (1..10) {
    $sysno = $sess->update_record(data       => $rec2,
				  recordType => 'grs.perl.pod',
				  groupName  => "demo1",
				  force      => 1,
				  );
}


$stat = $sess->end_trans;
ok(($stat->{inserted} == 1), "Inserted $stat->{inserted} records");
ok(($stat->{updated} == 120), "Updated $stat->{updated} records");
ok(($sysno > 0),"Inserted got valid sysno");

# ----------------------------------------------------------------------------
# Close session

$sess->commit;
$sess->close;

#!perl
# =============================================================================
# $Id: 03_record_update.t,v 1.10 2004-09-15 14:11:06 heikki Exp $
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

use Test::More tests => 15;

# ----------------------------------------------------------------------------
# Session opening and closing
BEGIN {
    use_ok('IDZebra');
    unlink("test03.log");
    IDZebra::logFile("test03.log");
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

my ($sysno, $stat, $ret);

$sess->init;
$sess->databases('demo1');

$sess->begin_trans;

($ret,$sysno) = $sess->insert_record(data       => $rec1,
			      recordType => 'grs.perl.pod',
			      );

ok(($ret == 0),"Must return ret=0 (OK)");

$stat = $sess->end_trans;
ok(($stat->{inserted} == 1), "Inserted 1 records");

$sess->begin_trans;
($ret,$sysno) = $sess->insert_record(data       => $rec2,
			      recordType => 'grs.perl.pod',
			      );
ok(($ret == 0),"Insert record ok");

$stat = $sess->end_trans;
ok(($stat->{inserted} == 1), "Inserted 1 records");


$sess->begin_trans;
($ret,$sysno) = $sess->update_record(data       => $rec3,
			      recordType => 'grs.perl.pod',
                              sysno => $sysno,
			      );

ok(($ret == 0),"update record ok");


$stat = $sess->end_trans;
ok(($stat->{inserted} == 0), "not inserted");
ok(($stat->{updated} == 1), "updated ok");
$sess->commit;

$sess->begin_trans;
#print STDERR "\nAbout to call delete. sysno=$sysno \n"; #!!!
($ret,$sysno) = $sess->delete_record( data       => $rec3,
                              sysno => $sysno,
			      recordType => 'grs.perl.pod',
			      );
ok(($ret == 0),"delete record ok");

#print STDERR "\nafter delete ret=$ret sysno=$sysno \n"; #!!!

$stat = $sess->end_trans;
ok(($stat->{inserted} == 0), "not inserted");
ok(($stat->{updated} == 0), "updated ok");
ok(($stat->{deleted} == 1), "deleted ok");
$sess->commit;



$sess->close;

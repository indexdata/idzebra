#!perl
# =============================================================================
# $Id: 03_record_update.t,v 1.9 2004-09-09 15:23:07 heikki Exp $
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

use Test::More tests => 18;

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

# ADAM: we must set database separately (cant be set from group)
$sess->databases('demo1');

$sess->begin_trans;
($ret,$sysno) = $sess->insert_record(data       => $rec1,
			      recordType => 'grs.perl.pod',
			      groupName  => "demo1",
			      );

print STDERR "\nAfter first insert_record. ret=$ret sysno=$sysno\n";

ok(($ret == 0),"Must return ret=0 (OK)");

$stat = $sess->end_trans;
ok(($stat->{inserted} == 1), "Inserted 1 records");
die;

$sess->begin_trans;
$sysno=-42;
$ret = $sess->insert_record(data       => $rec2,
			      recordType => 'grs.perl.pod',
			      groupName  => "demo1",
			      sysno => \$sysno,
			      );
ok(($ret == 0 && $sysno != 42),"Inserted record got valid sysno");

$stat = $sess->end_trans;
ok(($stat->{inserted} == 1), "Inserted 1 records");

$sess->commit;
$sess->close;

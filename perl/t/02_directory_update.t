#!perl
# =============================================================================
# $Id: 02_directory_update.t,v 1.4 2004-07-28 08:15:47 adam Exp $
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

use Test::More tests => 8;

# ----------------------------------------------------------------------------
# Session opening and closing
BEGIN {
    use_ok('IDZebra');
    unlink("test02.log");
    IDZebra::logFile("test02.log");
    use_ok('IDZebra::Session'); 
    use_ok('pod');
}


# ----------------------------------------------------------------------------
# Session opening and closing
my $sess = IDZebra::Session->open(configFile => 'demo/zebra.cfg',
				  groupName  => 'demo2');
isa_ok($sess,"IDZebra::Session");

SKIP: {
    skip "Takes a while", 5 if (0);

# ----------------------------------------------------------------------------
# init repository
$sess->init();
# ----------------------------------------------------------------------------
# repository upadte

# ADAM: we must set database separately (can't be used from group)
$sess->databases('demo2');

our $filecount = 8;
$sess->begin_trans;
$sess->update(path      =>  'lib');
my $stat = $sess->end_trans;

ok(($stat->{inserted} == $filecount), 
   "Inserted $stat->{inserted}/$filecount records");

# ADAM: we must set database separately (can't be used from group)
$sess->databases('demo1');

$sess->begin_trans;
$sess->update(groupName => 'demo1',
	      path      =>  'lib');

$stat = $sess->end_trans;
ok(($stat->{inserted} == $filecount), 
   "Inserted $stat->{inserted}/$filecount records");

$sess->begin_trans;
$sess->delete(groupName => 'demo1',
	      path      =>  'lib');
$stat = $sess->end_trans;
ok(($stat->{deleted} == $filecount), 
   "Deleted $stat->{deleted}/$filecount records");

$sess->begin_trans;
$sess->update(groupName => 'demo1',
	      path      =>  'lib');

$stat = $sess->end_trans;
ok(($stat->{inserted} == $filecount), 
   "Inserted $stat->{inserted}/$filecount records with shadow");

# ok(($sess->group->{databaseName} eq "demo2"),"Original group is selected"); deleted

# ----------------------------------------------------------------------------
# Close session
}
$sess->close;

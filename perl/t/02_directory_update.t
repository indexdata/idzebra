#!perl
# =============================================================================
# $Id: 02_directory_update.t,v 1.1 2003-03-03 00:44:39 pop Exp $
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

use Test::More tests => 9;

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
				  groupName => 'demo2');
isa_ok($sess,"IDZebra::Session");

SKIP: {
    skip "Takes a while", 5 if (0);

# ----------------------------------------------------------------------------
# init repository
$sess->init();

# ----------------------------------------------------------------------------
# repository upadte

our $filecount = 6;
$sess->begin_trans;
$sess->update(path      =>  'lib');
my $stat = $sess->end_trans;

ok(($stat->{inserted} == $filecount), 
   "Inserted $stat->{inserted}/$filecount records");

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
   "Inserted $stat->{inserted}/$filecount records");

ok(($sess->group->{databaseName} eq "demo2"),"Original group is selected");

# ----------------------------------------------------------------------------
# Close session
}
$sess->close;

#!perl -Tw
# =============================================================================
# $Id: 01_base.t,v 1.1 2003-03-03 00:44:39 pop Exp $
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
}

use pod;

# ----------------------------------------------------------------------------
# Session opening and closing
my $sess = IDZebra::Session->new(configFile => 'demo/zebra.cfg');
isa_ok($sess,"IDZebra::Session");

$sess->open();
ok(defined($sess->{zh}), "Zebra handle opened");

$sess->close();
ok(!defined($sess->{zh}), "Zebra handle closed");

$sess = IDZebra::Session->open(configFile => 'demo/zebra.cfg',
			       groupName  => 'demo1');
isa_ok($sess,"IDZebra::Session");
ok(defined($sess->{zh}), "Zebra handle opened");

# ----------------------------------------------------------------------------
# Record group tests
ok(($sess->group->{databaseName} eq "demo1"),"Record group is selected");

$sess->group(groupName => 'demo2');

ok(($sess->group->{databaseName} eq "demo2"),"Record group is selected");

# ----------------------------------------------------------------------------
# Close session

$sess->close;

#!perl
# =============================================================================
# $Id: 04_cql.t,v 1.1 2003-03-03 00:44:39 pop Exp $
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
    use IDZebra;
    IDZebra::logFile("test.log");
    use_ok('IDZebra::Session'); 
}


# ----------------------------------------------------------------------------
# Session opening and closing
my $sess = IDZebra::Session->new();

# ----------------------------------------------------------------------------
# CQL stuff
$sess->cqlmap('demo/cql.map');

$SIG{__WARN__} = \&catch_warn;

&check_cql($sess, "IDZebra", 0);
&check_cql($sess, "dc.title=IDZebra", 0);
&check_cql($sess, "dc.title=(IDZebra and Session)", 0);
&check_cql($sess, "dc.title=IDZebra and Session)", -1);
&check_cql($sess, "dc.title='IDZebra::Session'", 0);
&check_cql($sess, "anything=IDZebra", 15);

sub check_cql {
    my ($sess, $query, $exp) = @_;
    my ($rpn, $stat) = $sess->cql2pqf($query);
    if ($exp) {
	ok(($stat == $exp), "Wrong query ($stat): '$query'");
    } else {
	ok((($stat == 0) && ($rpn ne "")), "Good query query: '$query'");
    }
}


sub catch_warn {
    1;
}

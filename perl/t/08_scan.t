#!perl
# =============================================================================
# $Id: 08_scan.t,v 1.4 2004-09-20 15:59:48 heikki Exp $
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

use Test::More tests => 32;
#use Test::More skip_all => "Something rotten with scan.";

# ----------------------------------------------------------------------------
# Session opening and closing
BEGIN {
    use IDZebra;
    unlink("test08.log");
    IDZebra::logFile("test08.log");
  IDZebra::logLevel(15);
    use_ok('IDZebra::Session'); 
    use_ok('pod');
}


# ----------------------------------------------------------------------------
# Session opening and closing
my $sess = IDZebra::Session->open(configFile => 'demo/zebra.cfg',
				  groupName => 'demo1');

# ----------------------------------------------------------------------------
# Insert some test data
my $ret;
my $sysno;
my $F;
my $filecount=0;
$sess->init;

$sess->begin_trans;
$sess->databases('demo1', 'demo2');
for $F (<lib/IDZebra/*.pm>)
{
    ($ret,$sysno)=$sess->insert_record (file=>$F, recordType => 'grs.perl.pod');
    ok( $ret==0, "inserted $F");
    $filecount++;
}
$ret=$sess->end_trans;
ok($filecount>0,"Inserted files");
is($ret->{inserted},$filecount, "Inserted all");
$sess->databases('demo1');

# -----------------------------------------------------------------------------
# Scan titles in a single (default) database
$sess->begin_trans;
IDZebra::logMsg(2,"t08: Starting to scan");
my $sl0 = $sess->scan(expression => "\@attr 1=4 \@attr 6=1 a");
IDZebra::logMsg(2,"t08: scan done");

my @ent=$sl0->entries(position    => 1,
                      num_entries => 3);
my $nent=@ent;
is ($nent,3,"got 3 entries");

my $cnt= $sl0->num_entries;

is($cnt,3,"num_entries");
is($ent[0]->term(),"a", "first entry");
is($ent[1]->term(),"an", "second entry");
is($ent[2]->term(),"and", "third entry");

$sess->end_trans;
# -----------------------------------------------------------------------------
# Scan titles in a named database

my $sl1 = $sess->scan(expression => "\@attr 1=4 \@attr 6=2 a",
		      databases => [qw(demo1)]);
&test_list($sl1,$filecount, $filecount,1);

# FIXME - Should test multiple databases, but I can't get that to work


# -----------------------------------------------------------------------------
# Scan titles in a single and default database
my $sl2 = $sess->scan(expression => "\@attr 1=4 \@attr 6=2 a");
&test_list($sl2,$filecount, $filecount,1);


# -----------------------------------------------------------------------------
# Scan long list, with position...
my $sl3 = $sess->scan(expression  => "\@attr 1=1016 a");

my @entries = $sl3->entries(position    => 5,
			    num_entries => 10000);

my $count = $#entries + 1;
ok (($sl3->errCode == 0),"scan successful");
ok (($sl3->num_entries == $count),"fetched $count entries");
my $i = 1;
my $posok = 1;
foreach my $se (@entries) {
    $posok = 0 if ($se->position != $i); 
    $i++;
}
ok (($posok),"position of each term");


# -----------------------------------------------------------------------------
# Scan error
eval {my $sl4 = $sess->scan(expression => "\@attr 1=9999 a");};
ok (($@ ne ""),"Wrong scan die");
ok (($sess->errCode != 0), 
    "Error reported in session: ".$sess->errCode.
    " (". $sess->errString. ")");


# ----------------------------------------------------------------------------
# Close session
$sess->close;

# ============================================================================
sub test_list {
    my ($sl, $ecount, $occ, $offset) = @_;
    my @entries = $sl->entries();
    my $count = $#entries + 1;
    ok (($sl->errCode == 0),"scan successfull");
    ok (($sl->num_entries == $ecount),
	"number of entries is ".$sl->num_entries);
    is ($count,$sl->num_entries,"fetched $count entries");
    
    my $occcount=0; 
    my $posok = 1;
    my $i = $offset;
    foreach my $se (@entries) {
	$occcount += $se->occurrences();
	$posok = 0 if ($se->position != $i); 
	$i++;
    }
    
    is ($occcount, $occ,"occurrences: $occcount");
    ok (($posok),"position of each term");
}

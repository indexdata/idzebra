#!/usr/bin/perl

BEGIN {
    push (@INC,'demo','blib/lib','blib/arch');
}

use pod;

use Test::More tests => 15;

BEGIN { 
  use_ok('IDZebra::Session'); 
}

IDZebra::logFile("test.log");

#IDZebra::logLevel(15);

#IDZebra::init();

# ----------------------------------------------------------------------------
# Session opening and closing
my $sess = IDZebra::Session->new(configFile => 'demo/zebra.cfg');
isa_ok($sess,"IDZebra::Session");

$sess->open();
ok(defined($sess->{zh}), "Zebra handle opened");
$sess->close();
ok(!defined($sess->{zh}), "Zebra handle closed");


my $sess = IDZebra::Session->open(configFile => 'demo/zebra.cfg',
				  groupName  => 'demo1');
isa_ok($sess,"IDZebra::Session");
ok(defined($sess->{zh}), "Zebra handle opened");

# ----------------------------------------------------------------------------
# Record group tests

ok(($sess->group->{databaseName} eq "demo1"),"Record group is selected");

$sess->group(groupName => 'demo2');

ok(($sess->group->{databaseName} eq "demo2"),"Record group is selected");

# ----------------------------------------------------------------------------
# init repository
$sess->init();

# ----------------------------------------------------------------------------
# repository upadte
$sess->begin_trans;
$sess->update(path      =>  'lib');
my $stat = $sess->end_trans;

ok(($stat->{inserted} == 6), "Inserted 6 records");

$sess->begin_trans;
$sess->update(groupName => 'demo1',
	      path      =>  'lib');

my $stat = $sess->end_trans;
ok(($stat->{updated} == 6), "Updated 6 records");

$sess->begin_trans;
$sess->delete(groupName => 'demo1',
	      path      =>  'lib');
my $stat = $sess->end_trans;
ok(($stat->{deleted} == 6), "Deleted 6 records");

$sess->begin_trans;
$sess->update(groupName => 'demo1',
	      path      =>  'lib');

my $stat = $sess->end_trans;
ok(($stat->{inserted} == 6), "Inserted 6 records");

ok(($sess->group->{databaseName} eq "demo2"),"Original group is selected");

# ----------------------------------------------------------------------------
# per record update
my $rec1=`cat lib/IDZebra/Data1.pm`;
my $rec2=`cat lib/IDZebra/Filter.pm`;

$sess->begin_trans;
my $s1=$sess->update_record(data       => $rec1,
			    recordType => 'grs.perl.pod',
			    groupName  => "demo1",
			    );

#my $s2=$sess->update_record(data       => $rec2);
#					recordType => "grs.perl.pod");


#my $s3=$sess->update_record(file       => "lib/IDZebra/Data1.pm");



my $stat = $sess->end_trans;
ok(($stat->{updated} == 1), "Updated 1 records");

#$sess->cqlmap("cql.map");
#print STDERR $sess->cql2pqf("job.id <= 5");
#print STDERR $sess->cql2pqf("job.id=5 and dc.title=computer");
#print STDERR "RES:$res\n";

$sess->close;
ok(!defined($sess->{zh}), "Zebra handle closed");


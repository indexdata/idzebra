#!/usr/bin/perl

BEGIN {
    push (@INC,'demo','blib/lib','blib/arch');
}
use Test::More tests => 3;
use Data::Dumper;
use IDZebra::Logger qw(:flags :calls);
use IDZebra::Repository;
use pod;

BEGIN { 
  use_ok('IDZebra'); 
  use_ok('IDZebra::Service'); 
  use_ok('IDZebra::Data1'); 
}

mkdir ("demo/tmp");
mkdir ("demo/register");
mkdir ("demo/lock");

#Zebra::API::LogFile("a.log");


#my $arr = IDZebra::give_me_array("strucc",6);

#print "$arr\n";

#for (@arr) {print "$_\n";}

#exit;

IDZebra::init();

chdir('demo');

my $service = IDZebra::Service->start('zebra.cfg');
my $sess = $service->openSession;
#my $sess = $service->createSession;
#$sess->open;
#my $session = IDZebra::open($service);
#IDZebra::close($session);
#IDZebra::stop($service);
#$sess->close;

my $rec1=`cat ../lib/IDZebra/Data1.pm`;
my $rec2=`cat ../lib/IDZebra/Filter.pm`;

#$sess->Repository->readConfig;
$sess->Repository->readConfig("","pm");

$sess->begin_trans;

#$sess->Repository->update(databaseName => 'Default',
#			  path  => '/usr/local/work/cvs/zebra/perl/lib');
my $s1 = $sess->Repository->update_record($rec1,0,"","Data1.pm");
my $s2 = $sess->Repository->update_record($rec2,0,"","Filter.pm");
print STDERR "s1:$s1, s2:$s2\n";

$sess->end_trans;
#$sess->begin_trans;
#$sess->Repository->delete_record($rec1,0,"","Data1.pm");
#$sess->end_trans;

$sess->select_databases('Default');

goto scan;

$sess->begin_read;
#print STDERR "Hits:", $sess->search_pqf('@or @attr 1=4 Filter @attr 1=4 Data1','test_1'), "\n";
#print STDERR "Hits:", $sess->search_pqf('@or @attr 1=4 Filter @attr 1=4 Data1','test_1'), "\n";

my $rs1 = $sess->search_pqf('@or @attr 1=4 Filter @attr 1=4 Data1','test_1');
print STDERR "Rs1 '$rs1->{name}' has $rs1->{recordCount} hits\n";

my $rs2 = $sess->search_pqf('@or @attr 1=4 Filter @attr 1=4 Data1','test_2');
#print STDERR "Rs2 '$rs2->{name}' has $rs2->{recordCount} hits\n";

my $rs3 = $sess->sortResultsets ('1=4 id','test_3',($rs1));
#print STDERR "Rs3 '$rs3->{name}' has $rs3->{recordCount} hits\n";
#print STDERR "Rs3 '$rs3->{name}' error $rs3->{errCode}: $rs3->{errString}\n";

$rs1->sort('1=4 id');

#for ($i=1; $i<100000; $i++) {
my @recs1 = $rs1->records(from=>1,to=>2);
#}
#my $res=$sess->retrieve_records('test_1',1,1);

$sess->end_read;


#IDZebra::describe_recordGroup($rep->{rg});
#$rep->update;
#    print "HOW did we got back???\n";

scan:

my $so = IDZebra::ScanObj->new;
$so->{position} = 1;
$so->{num_entries} = 20;
$so->{is_partial} = 0;
#print STDERR "Pos:$so->{position}\nNum:$so->{num_entries}\nPartial:$so->{is_partial}\n";

IDZebra::scan_PQF($sess->{zh}, $so,
		  $sess->{odr_output}, 
		  "\@attr 1=4 a");

#print STDERR "Pos:$so->{position}\nNum:$so->{num_entries}\nPartial:$so->{is_partial}\n";

for ($i=1; $i<=$so->{num_entries}; $i++) {
    my $se = IDZebra::getScanEntry($so, $i);
    print STDERR "$se->{term} ($se->{occurrences})\n";
}

$sess->close;
$service->stop;
			  
foreach my $rec (@recs1) {
    foreach my $line (split (/\n/, $rec->{buf})) {
	if ($line =~ /^package/) { print STDERR "$line\n";}
    }
}

#$rep->{groupName} = "Strucc";
#$rep->describe();

sub test_data1 {
    $m = IDZebra::nmem_create();
    my $d1=IDZebra::Data1->new($m,$IDZebra::DATA1_FLAG_XML);
    my $root=$d1->mk_root('strucc');
    my $tag1 = $d1->mk_tag($root,'emu',('asd' => 1,
					'bsd' => 2));
    my $tag2 = $d1->mk_tag($root,'emu');
    $d1->pr_tree($root);
    IDZebra::nmem_destroy($m);
    $d1->DESTROY();
}

IDZebra::DESTROY;

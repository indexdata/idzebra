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

#Zebra::API::LogFile("a.log");


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
#$sess->begin_read;
print STDERR "Hits:", $sess->search_pqf('@or @attr 1=4 Filter @attr 1=4 Data1','test_1'), "\n";
print STDERR "Hits:", $sess->search_pqf('@or @attr 1=4 Filter @attr 1=4 Data1','test_1'), "\n";
print STDERR "Hits:", $sess->search_pqf('@or @attr 1=4 Filter @attr 1=4 Data1','test_1'), "\n";
#$sess->end_read;

#$sess->commit;
#IDZebra::describe_recordGroup($rep->{rg});
#$rep->update;
#    print "HOW did we got back???\n";
$sess->close;
$service->stop;
			  

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

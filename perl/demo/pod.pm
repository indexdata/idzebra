#!/usr/bin/perl
use strict;
# ----------------------------------------------------------------------------
# A dummy example to demonstrate perl filters for zebra. This is going to
# extract information from the .pm perl module files.
# ----------------------------------------------------------------------------
package pod;

use IDZebra::Filter;
use IDZebra::Data1;
use Pod::Text;
our @ISA=qw(IDZebra::Filter);
1;


sub init {
    # Initialization code may come here
}

sub process {
    my ($self, $d1) = @_;

    my $tempfile_in = "/tmp/strucc.in";
    my $tempfile_out = "/tmp/strucc.out";
    my $parser = Pod::Text->new (sentence => 0, width => 78);

    my $r1=$d1->mk_root('pod');    
    my $root=$d1->mk_tag($r1,'pod');

    # This is dirty... Pod::Parser doesn't seems to support 
    # parsing a string, so we have to write the whole thing out into a 
    # temporary file
    open (TMP, ">$tempfile_in");
    print TMP $self->readall(10240);
    close (TMP);

    $parser->parse_from_file ($tempfile_in, $tempfile_out);

    my $section;
    my $data;
    open (TMP, "$tempfile_out");
    while(<TMP>) {
	chomp;
	if (/^([A-Z]+)\s*$/) {
	    my $ss = $1;
	    if ($section) {
		my $tag = $d1->mk_tag($root,$section);
		$d1->mk_text($tag,$data) if ($data);
	    }
	    $section = $ss;
	    $data = "";
	    next;
	}
	s/^\s+|\s+$//g;
	$data.="$_\n";
    }

    if ($section) { 
	my $tag = $d1->mk_tag($root,$section);
	$d1->mk_text($tag,$data) if ($data);
    }
    close (TMP);
    
    return ($r1);
}

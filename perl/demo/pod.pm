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
use Symbol qw(gensym);
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

    # Get the input "file handle"
    my $inf = $self->get_fh;

    # Create a funny output "file handle"
    my $outf = gensym;
    tie (*$outf,'MemFile');

    $parser->parse_from_filehandle ($inf, $outf);

    my $section;
    my $data;
    while(<$outf>) {
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
    return ($r1);
}

# ----------------------------------------------------------------------------
# Package to collect data as an output file from stupid modules, who can only
# write to files...
# ----------------------------------------------------------------------------
package MemFile;

sub TIEHANDLE {
    my $class = shift;
    my $self = {};
    bless ($self,$class);
    $self->{buff} = "";
    return ($self);
}

sub PRINT {
    my $self = shift;
    for (@_) {
	$self->{buff} .= $_;
    }
}

sub READLINE {
    my $self = shift;
    my $res;
    return (undef) unless ($self->{buff});
    ($res,$self->{buff}) = split (/\n/,$self->{buff},2);
    return ($res."\n");
}

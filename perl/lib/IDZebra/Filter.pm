#!/usr/bin/perl
# ============================================================================
# Zebra perl API header
# =============================================================================
use strict;
use Carp;
# ============================================================================
package IDZebra::Filter;
use IDZebra;
use IDZebra::Data1;
use IDZebra::Logger qw(:flags :calls);
use Symbol qw(gensym);
#use Devel::Leak;

our $SAFE_MODE = 1;

BEGIN {
    IDZebra::init(); # ??? Do we need that at all (this is jus nmem init...)
}

1;
# -----------------------------------------------------------------------------
# Class constructor
# -----------------------------------------------------------------------------
sub new {
    my ($proto,$context) = @_;
    my $class = ref($proto) || $proto;
    my $self = {};
    $self->{context} = $context;
    bless ($self, $class);
    return ($self);
}

# -----------------------------------------------------------------------------
# Callbacks
# -----------------------------------------------------------------------------
sub _process {
    my ($self) = @_;

#    if ($self->{dl}) {
#	print STDERR "LEAK",Devel::Leak::CheckSV($self->{dl}),"\n";
#    }

#    print STDERR "LEAK",Devel::Leak::NoteSV($self->{dl}),"\n";

    # This is ugly... could be passed as parameters... but didn't work.
    # I don't know why...
    my $dh  = IDZebra::grs_perl_get_dh($self->{context});
    my $mem = IDZebra::grs_perl_get_mem($self->{context});
    my $d1  = IDZebra::Data1->get($dh,$mem);

    my $rootnode;
    if ($SAFE_MODE) {
	eval {$rootnode = $self->process($d1)};
	if ($@) {
	    logf(LOG_WARN,"Error processing perl filter:%s\n",$@);
	}
    } else {
	$rootnode = $self->process($d1);
    }
    IDZebra::grs_perl_set_res($self->{context},$rootnode);
    return (0);
}

sub _store_buff {
    my ($self, $buff) = @_;
    $self->{_buff} = $buff;
}

# -----------------------------------------------------------------------------
# API Template - These methods should be overriden by the implementing class.
# -----------------------------------------------------------------------------
sub init {
    # This one is called once, when the module is loaded. Not in
    # object context yet!!!
}

sub process {
    my ($self, $d1) = @_;
    # Just going to return a root node.
    return ($d1->mk_root('empty'));  
}

# -----------------------------------------------------------------------------
# Testing
# -----------------------------------------------------------------------------
sub test {
    my ($proto, $file, %args) = @_;

    my $class = ref($proto) || $proto;
    my $self = {};
    bless ($self, $class);

    my $th;
    open ($th, $file) || croak ("Cannot open $file");

    $self->{testh} = $th;
    
    my $m = IDZebra::nmem_create();
    my $d1=IDZebra::Data1->new($m,$IDZebra::DATA1_FLAG_XML);
    if ($args{tabPath}) { $d1->tabpath($args{tabPath}); }
    if ($args{tabRoot}) { $d1->tabroot($args{tabRoot}); }

    my $rootnode = $self->process($d1);
    $d1->pr_tree($rootnode);
    $d1->free_tree($rootnode);
    $d1 = undef;

    close ($th);
    $self->{testh} = undef;

}

# -----------------------------------------------------------------------------
# Utility calls
# -----------------------------------------------------------------------------
sub readf {
    my ($self, $buff, $len, $offset) = @_;
    $buff = "";
    if ($self->{testh}) {
	return (read($self->{testh},$_[1],$len,$offset));
    } else {
	my $r = IDZebra::grs_perl_readf($self->{context},$len);
	if ($r > 0) {
	    $buff = $self->{_buff};
	    $self->{_buff} = undef;	
	}
	return ($r);
    }
}

sub readline {
    my ($self) = @_;

    my $r = IDZebra::grs_perl_readline($self->{context});
    if ($r > 0) {
	my $buff = $self->{_buff};
	$self->{_buff} = undef;	
	return ($buff);
    }
    return (undef);
}

sub getc {
    my ($self) = @_;
    return(IDZebra::grs_perl_getc($self->{context}));
}

sub get_fh {
    my ($self) = @_;
    if ($self->{testh}) {
	return ($self->{testh});
    } else {
	my $fh = gensym;
	tie (*$fh,'IDZebra::FilterFile', $self);
	return ($fh);
    }
}

sub readall {
    my ($self, $buffsize) = @_;
    my $r; 
    my $res = ""; 

    do {
	if ($self->{testh}) {
	    $r = read($self->{testh}, $self->{_buff}, $buffsize);
	} else {
	    $r = IDZebra::grs_perl_readf($self->{context},$buffsize);
	}
	if ($r > 0) {
	    $res .= $self->{_buff};
	    $self->{_buff} = undef;	
	}
    } until ($r <= 0);

    return ($res);
}

sub seekf {
    my ($self, $offset) = @_;
    if ($self->{testh}) {
	# I'm not sure if offset is absolute or relative here...
	return (seek ($self->{testh}, $offset, $0));
    } else { 
	return (IDZebra::grs_perl_seekf($self->{context},$offset)) ; 
    }
}

sub tellf {
    my ($self) = @_;
    if ($self->{testh}) {
	# Not implemented
    } else {
	return (IDZebra::grs_perl_seekf($self->{context})); 
    }
}

sub endf {
    my ($self, $offset) = @_;
    if ($self->{testh}) {
	# Not implemented
    } else {
        IDZebra::grs_perl_endf($self->{context},$offset);	
    }
}
# ----------------------------------------------------------------------------
# The 'virtual' filehandle for zebra extract calls
# ----------------------------------------------------------------------------
package IDZebra::FilterFile;
require Tie::Handle;

our @ISA = qw(Tie::Handle);

sub TIEHANDLE {
    my $class = shift;
    my $self = {};
    bless ($self, $class);
    $self->{filter} = shift;
    return ($self);
}

sub READ {
    my $self = shift;
    return ($self->{filter}->readf(@_));
}

sub READLINE {
    my $self = shift;
    return ($self->{filter}->readline());
}

sub GETC {
    my $self = shift;
    return ($self->{filter}->getc());
}

sub EOF {
    croak ("EOF not implemented");
}

sub TELL {
    croak ("TELL not implemented");
}

sub SEEK {
    croak ("SEEK not implemented");
}

sub CLOSE {
    my $self = shift;
}


__END__

=head1 NAME

IDZebra::Filter - A superclass of perl filters for Zebra

=head1 SYNOPSIS

   package MyFilter;

   use IDZebra::Filter;
   our @ISA=qw(IDZebra::Filter);

   ...

   sub init {
 
   }

   sub process {
       my ($self,$d1) = @_;
       my $rootnode=$d1->mk_root('meta');    
       ...
       return ($rootnode)
   }

=head1 DESCRIPTION

When Zebra is trying to index/present a record, needs to extract information from it's source. For some types of input, "hardcoded" procedures are defined, but you have the opportunity to write your own filter code in Tcl or in perl.

The perl implementation is nothing but a package, deployed in some available location for Zebra (in I<profilePath>, or in PERL_INCLUDE (@INC)). This package is interpreted once needed, a filter object is created, armored with knowledge enough, to read data from the source, and to generate data1 structures as result. For each individual source "files" the  process method is called.

This class is supposed to be inherited in all perl filter classes, as it is providing a way of communication between the filter code and the indexing/retrieval process.

=head1 IMPLEMENTING FILTERS IN PERL

All you have to do is to create a perl package, using and inheriting this one (IDZebra::Filter), and implement the "process" method. The parameter of this call is an IDZebra::Data1 object, representing a data1 handle. You can use this, to reach the configuration information and to build your data structure. What you have to return is a data1 root node. To create it:

   my $rootnode=$d1->mk_root('meta');    

where 'meta' is the abstract syntax identifier (in this case Zebra will try to locate meta.abs, and apply it). Then just continue to build the structure. See i<IDZebra::Data1> for details.

In order to get the input stream, you can use "virtual" file operators (as the source is not necessairly a file):

=over 4

=item B<readf($buf,$len,$offset)>

Going to read $len bytes of data from offset $offset into $buff

=item B<readline()>

Read one line

=item B<getc()>

Get one character (byte)

=item B<readall($bufflen)>

Read the entire stream, by reading $bufflen bytes at once

=item B<seekf($offset)>

Position to $offset

=item B<tellf()>

Tells the current offset (?)

=item B<endf($offset)>

???

=back

You can optionally get a virtual perl filehandle as well:

  my $fh = $self->get_fh();
  while (<$fh>) {
    # ...
  }

Note, that the virtual filehandle implementation is not finished yet, so some applications may have problems using that. See TODO.

You can implement an init call for your class. This call is not going to be called in object, but in class context. Stupid, eh?

=head1 TEST YOUR PERL FILTER

You can check the functionality of your filter code, by writing a small test program like

  
   use pod;
   $res =pod->test($ARGV[0],
                   (tabPath=>'.:../../tab:../../../yaz/tab'));

This will try to apply the filter on the file provided as argument, and display the generated data1 structure. However, there are some differences, when running a filter in test mode: 
 - The include path is not applied from tabPath
 - the tellf, and endf functions are not implemented (just ignored)

=head1 CONFIGURE ZEBRA TO USE A PERL FILTER

This is quite simple. Read the Zebra manual, and follow the instructions to create your zebra.cfg. For your I<recordType> choose 'grs.perl.<YourFilterClass>'. 
Copy your filter module (YourFilterClass.pm) to a directory listed in I<profilePath>. i<profilePath> is added to @INC, when interpreting your package: so if you need to load modules from different locations than the default perl include path, just add these directories.

=head1 MISC OPTIONS

By default, filter code (process method) is executed within an eval {} block, and only a warning is sent to the log, if there is an error. To turn this option off, set B<$IDZebra::Filter::SAFE_MODE> to B<0>;

=head1 TODO

Finish virtual (tied) filehandle methods (SEEK, EOF, TELL);

=head1 COPYRIGHT

Fill in

=head1 AUTHOR

Peter Popovics, pop@technomat.hu

=head1 SEE ALSO

IDZebra, IDZebra::Data1, Zebra documentation

=cut

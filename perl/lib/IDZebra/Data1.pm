#!/usr/bin/perl
# ============================================================================
# Zebra perl API header
# =============================================================================
use strict;
use Carp;
# =============================================================================
package IDZebra::Data1;
use IDZebra;
1;

# -----------------------------------------------------------------------------
# Class constructors and destructor
# -----------------------------------------------------------------------------
sub get {
    my ($proto, $handle, $mem) = @_;
    my $class = ref($proto) || $proto;
    my $self = {};
    bless ($self,$class);
    
    $self->{dh} = $handle;
    $self->{mem} = $mem;
    # data1_destroy is not going to be called, when object is released
    $self->{dflag} = 0;  
    
    return ($self);
}

sub new {
    my ($proto, $mem, $flag) = @_;
    my $class = ref($proto) || $proto;
    my $self = {};

    bless ($self,$class);

    unless ($self->{dh} = IDZebra::data1_createx($flag)) {
	croak ("Cannot create data1 handle");
    }

    # data1_destroy going to be called, when object is released
    $self->{dflag} = 1;

    unless ($self->{mem} = $mem) {
	croak ("Missing NMEM handle");
    }

    return ($self);
}

sub DESTROY {
    my $self = shift;
    if ($self->{dh}) {
	if ($self->{dflag}) { IDZebra::data1_destroy($self->{dh}) }
	$self->{dh} = undef;
    }
}

# -----------------------------------------------------------------------------
# Profile information
# -----------------------------------------------------------------------------
# Not usable...
sub path_fopen {
    my ($self, $file, $mode) = @_;
    return (IDZebra::data1_path_fopen ($self->{dh}, $file, $mode));
}

sub tabpath {
    my ($self, $path) = @_;
    if (defined($path)) { IDZebra::data1_set_tabpath($self->{dh}, $path); }
    return (IDZebra::data1_get_tabpath($self->{dh}));
}

sub tabroot {
    my ($self, $path) = @_;
    if (defined($path)) { IDZebra::data1_set_tabroot($self->{dh}, $path); }
    return (IDZebra::data1_get_tabroot($self->{dh}));
}

# -----------------------------------------------------------------------------
# D1 Structure manipulation
# -----------------------------------------------------------------------------
sub mk_root {
    my ($self, $name) = @_;
    return (IDZebra::data1_mk_root($self->{dh}, $self->{mem}, $name));
}

sub set_root {
    my ($self, $node, $name) = @_;
    IDZebra::data1_set_root($self->{dh}, $node, $self->{mem}, $name);
}

sub mk_tag {
    my ($self, $parent, $tag, @attributes) = @_;
    return (IDZebra::data1_mk_tag($self->{dh}, $self->{mem},
				  $tag, \@attributes, $parent)); 
}

sub tag_add_attr {
    my ($self, $node, @attributes) = @_;
    IDZebra::data1_tag_add_attr ($self->{dh}, $self->{mem},
				 $node, \@attributes);
}

sub mk_text {
    my ($self, $parent, $text) = @_;
    $text = "" unless defined ($text);
    return (IDZebra::data1_mk_text($self->{dh}, $self->{mem},
				   $text, $parent)); 
}

sub mk_comment {
    my ($self, $parent, $text) = @_;
    return (IDZebra::data1_mk_comment($self->{dh}, $self->{mem},
				   $text, $parent)); 
}

sub mk_preprocess {
    my ($self, $parent, $target, @attributes) = @_;
    return (IDZebra::data1_mk_preprocess($self->{dh}, $self->{mem},
					 $target, \@attributes, $parent)); 
}


sub pr_tree {
    my ($self, $node) = @_;
    IDZebra::data1_print_tree($self->{dh}, $node);
}

sub free_tree {
    my ($self, $node) = @_;
    IDZebra::data1_free_tree($self->{dh}, $node);
} 

# =============================================================================

__END__

=head1 NAME

IDZebra::Data1 - OO Aproach interface for data1 structures

=head1 SYNOPSIS

   use IDZebra::Data1;

   my $m = IDZebra::nmem_create();
   my $d1=IDZebra::Data1->new($m,$IDZebra::DATA1_FLAG_XML);
   my $root = $d1->mk_root('ostriches');
   my $tag  = $d1->mk_tag($root,'emu',('speed'  => 120,
                                       'height' => 110));
   $d1->pr_tree($root);

=head1 DESCRIPTION

I never managed to understand data1 entirely. Probably Adam, or someone else from IndexData could write a more deep introduction here. However here are some ideas:

Data1 structures are used in zebra to represent structured data. You can map an xml structure, a marc record, or anything in D1. These structures are built by different filters - this is called "extraction" in zebra's code. 

When zebra asks a filter to extract a file, it provides a data1 handle, which can be used to

  - reach profile information, provided in zebra.cfg, and other refered 
    configuration files, (like tab path).

  - build data1 structures

In one word, a data1 handle is a kind of context in the d1 API. This handle is represented here as a IDZebra::Data1 object. When you implement a filter, you'll get this object ready for use, otherwise, you'll have to prepare an NMEM handle, and call the constructor:

   my $m = IDZebra::nmem_create();
   my $dh = IDZebra::Data1->new($m,$IDZebra::DATA1_FLAG_XML);

What is FLAG_XML? I don't know exactly. You don't have to worry about it, it's already set, if you implement a filter. 

=head1 PROFILE INFORMATION

=item $d1->tabpath([$path])

Set and/or get the tab path. This is a colon separated list of directories, where different configuration files can be found.

=item $d1->tabroot([$path])

Set and/or get the tab root.

=head1 BUILDING DATA STRUCTURES

It's obvious, that first of all you have to create a root node:

   my $r1 = $d1->mk_root('pod');    

This is going to initialize the abstract syntax "pod" (trying to open and parse pod.abs). I don't know why exactly, but then, you'll have to create a root tag as well, under the same name.

   my $root=$d1->mk_tag($r1,'pod');

Then just continue, to add child nodes, as tags, text nodes... to your structure. 

=item $d1->mk_root($name)

Create a root node, with the given name. (name is type in d1 terminology?)

=item $d1->set_root($node, $name)

Makes an existing node into root node, under the given name

=item $d1->mk_tag($parent, $name, [@attributes])

Add a tag to the parent node, with the given name and attributes. For example:

   my $tag  = $d1->mk_tag($root,'emu',('speed'  => 120,
                                       'height' => 110));

=item $d1->tag_add_attr($node, @attributes)

Add attributes to an existing node

=item $d1->mk_text($parent, $text)

Add a text node to the given parent

=item $d1->mk_comment($parent, $text)

Add a comment node to the given parent

=item $d1->mk_preprocess($parent, $target, $attributes)

???

=item $d1->pr_tree($node)

Prints data1 tree on STDOUT;

=item $d1->free_tree($node)

Destroys a data1 node structure;

=head1 COPYRIGHT

Fill in

=head1 AUTHOR

Peter Popovics, pop@technomat.hu

=head1 SEE ALSO

IDZebra, Zebra documentation

=cut

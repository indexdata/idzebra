#!/usr/bin/perl -w

use strict;


package Node;

sub new {
    my $class = shift();
    my($name, $id, $parent, $note) = @_;

    my $this = bless { name => $name,
		       id => $id,
		       parent => $parent,
		       children => [],
		       note => $note }, $class;
    push @{ $parent->{children} }, $this
	if defined $parent;

    return $this;
}

sub walk {
    my $this = shift();
    my($coderef) = @_;

    &$coderef($this);
    foreach my $child (@{ $this->{children} }) {
	$child->walk($coderef)
    }
}

sub write_zthes {
    my $this = shift();

    print "<Zthes>\n";
    $this->write_term(1);
    my $note = $this->{note};
    print " <termNote>$note</termNote>\n" if defined $note;
    my $parent = $this->{parent};
    if (defined $parent) {
	$parent->write_relation('BT');
    }
    foreach my $child (@{ $this->{children} }) {
	$child->write_relation('NT');
    }
    print "</Zthes>\n";
}

sub write_relation {
    my $this = shift();
    my($type) = @_;

    print " <relation>\n";
    print "  <relationType>$type</relationType>\n";
    $this->write_term(2);
    print " </relation>\n";
}

sub write_term {
    my $this = shift();
    my($level) = @_;

    print ' ' x $level, "<termId>", $this->{id}, "</termId>\n";
    print ' ' x $level, "<termName>", $this->{name}, "</termName>\n";
    print ' ' x $level, "<termType>PT</termType>\n";
}


package main;

my @stack;
my $id = 1;

while (<>) {
    chomp();
    s/\t/        /g;
    s/^( *)//;
    my $level = length($1);
    s/^\*+ //;
    my $note = undef;
    if (s/[ \t]+(.*)//) {
	$note = $1;
    }
    my $parent = undef;
    $parent = $stack[$level-1] if $level > 0;
    $stack[$level] = new Node($_, $id++, $parent, $note);
}

$stack[0]->walk(\&Node::write_zthes);

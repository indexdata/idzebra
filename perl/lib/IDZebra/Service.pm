#!/usr/bin/perl
# ============================================================================
# Zebra perl API header
# =============================================================================
use strict;
use Carp;
# ============================================================================
package IDZebra::Service;
use IDZebra;
use IDZebra::Session;
use IDZebra::Logger qw(:flags :calls);
use Scalar::Util qw(weaken);

our @ISA = qw(IDZebra::Logger);

1;
# -----------------------------------------------------------------------------
# Class constructors, destructor
# -----------------------------------------------------------------------------
sub new {
    my ($proto,$configName) = @_;
    my $class = ref($proto) || $proto;
    my $self = {};
    $self->{configName} = $configName;
    $self->{sessions} = {};
    $self->{sessc} = 0;
    bless ($self, $class);
    return ($self);
}

sub start {
    my ($proto,$configName) = @_;
    my $self = {};
    if (ref($proto)) { $self = $proto; } else { 
	$self = $proto->new($configName);
    }
    unless (defined($self->{zs})) {
	$self->{zs} = IDZebra::start($self->{configName});
    }
    return ($self);
}

sub stop {
    my ($self) = @_;
    foreach my $sess (values(%{$self->{sessions}})) {
	$sess->close;
    }
    IDZebra::stop($self->{zs}) if ($self->{zs});    
    $self->{zs} = undef;
}

sub DESTROY {
    my ($self) = @_;
    $self->stop; 
}
# -----------------------------------------------------------------------------
# Session management
# -----------------------------------------------------------------------------
sub createSession {
    my ($self) = @_;
    my $session = IDZebra::Session->new($self);
    $self->{sessions}{$self->{sessc}} = $session;
    weaken ($self->{sessions}{$self->{sessc}});
    $self->{sessc}++;
    return($session);
}

sub openSession {
    my ($self) = @_;
    my $session = IDZebra::Session->open($self);
    $self->{sessions}{$self->{sessc}} = $session;
    weaken ($self->{sessions}{$self->{sessc}});
    $self->{sessc}++;
    return($session);
}

__END__

=head1 NAME

IDZebra::Service - 

=head1 SYNOPSIS

=head1 DESCRIPTION

=head1 COPYRIGHT

Fill in

=head1 AUTHOR

Peter Popovics, pop@technomat.hu

=head1 SEE ALSO

IDZebra, IDZebra::Data1, Zebra documentation

=cut

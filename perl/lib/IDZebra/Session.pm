#!/usr/bin/perl
# ============================================================================
# Zebra perl API header
# =============================================================================
use strict;
use Carp;
# ============================================================================
package IDZebra::Session;
use IDZebra;
use IDZebra::Logger qw(:flags :calls);
use IDZebra::Repository;
use Scalar::Util;

our @ISA = qw(IDZebra::Logger);

1;
# -----------------------------------------------------------------------------
# Class constructors, destructor
# -----------------------------------------------------------------------------
sub new {
    my ($proto,$service) = @_;
    my $class = ref($proto) || $proto;
    my $self = {};
    $self->{service} = $service;
    $self->{sessionID} = $service->{sessc};
    bless ($self, $class);
    return ($self);
}

sub open {
    my ($proto,$service) = @_;
    my $self = {};
    if (ref($proto)) { $self = $proto; } else { 
	$self = $proto->new($service);
    }
    unless (defined($self->{zh})) {
	$self->{zh}=IDZebra::open($self->{service}{zs}) if ($self->{service}); 
    }   
    $self->Repository(); # Make a dummy record group

    $self->{odr_input} = IDZebra::odr_createmem($IDZebra::ODR_DECODE);
    $self->{odr_output} = IDZebra::odr_createmem($IDZebra::ODR_ENCODE);

    return ($self);
}

sub close {
    my ($self) = @_;

    if ($self->{zh}) {
        IDZebra::close($self->{zh});
	$self->{zh} = undef;
    }

    if ($self->{odr_input}) {
        IDZebra::odr_destroy($self->{odr_input});
	$self->{odr_input} = undef;  
    }

    if ($self->{odr_output}) {
        IDZebra::odr_destroy($self->{odr_output});
	$self->{odr_output} = undef;  
    }

    delete($self->{service}{sessions}{$self->{sessionID}});
    delete($self->{service});
}

sub DESTROY {
    my ($self) = @_;
    print STDERR "Destroy_session\n";
    $self->close; 
}
# -----------------------------------------------------------------------------
# Error handling
# -----------------------------------------------------------------------------
sub errCode {
    my ($self) = @_;
    return(IDZebra::errCode($self->{zh}));
}

sub errString {
    my ($self) = @_;
    return(IDZebra::errString($self->{zh}));
}

sub errAdd {
    my ($self) = @_;
    return(IDZebra::errAdd($self->{zh}));
}

# -----------------------------------------------------------------------------
# Transaction stuff
# -----------------------------------------------------------------------------
sub begin_trans {
    my ($self) = @_;
    unless ($self->{trans_started}) {
        $self->{trans_started} = 1;
        IDZebra::begin_trans($self->{zh});
    }
}

sub end_trans {
    my ($self) = @_;
    if ($self->{trans_started}) {
        $self->{trans_started} = 0;
        IDZebra::end_trans($self->{zh});
    }
}

sub shadow_enable {
    my ($self, $value) = @_;
    if ($#_ > 0) { IDZebra::set_shadow_enable($self->{zh},$value); }
    return (IDZebra::get_shadow_enable($self->{zh}));
}

sub commit {
    my ($self) = @_;
    if ($self->shadow_enable) {
	return(IDZebra::commit($self->{zh}));
    }
}

# -----------------------------------------------------------------------------
# We don't really need that...
# -----------------------------------------------------------------------------
sub odr_reset {
    my ($self, $name) = @_;
    if ($name !~/^(input|output)$/) {
	croak("Undefined ODR '$name'");
    }
  IDZebra::odr_reset($self->{"odr_$name"});
}

# -----------------------------------------------------------------------------
# Init/compact
# -----------------------------------------------------------------------------
sub init {
    my ($self) = @_;
    return(IDZebra::init($self->{zh}));
}

sub compact {
    my ($self) = @_;
    return(IDZebra::compact($self->{zh}));
}

# -----------------------------------------------------------------------------
# Repository stuff
# -----------------------------------------------------------------------------
sub Repository {
    my ($self, %args) = @_;
    if (!$self->{rep}) {
	$self->{rep} = IDZebra::Repository->new($self, %args);
    } else {
	$self->{rep}->modify(%args);
    }

    return ($self->{rep});
}

# -----------------------------------------------------------------------------
# Search and retrieval
# -----------------------------------------------------------------------------
sub select_databases {
    my ($self, @databases) = @_;
    return (IDZebra::select_databases($self->{zh}, 
				      ($#databases + 1), 
				      \@databases));
}

sub begin_read {
    my ($self) =@_;
    return(IDZebra::begin_read($self->{zh}));
}

sub end_read {
    my ($self) =@_;
    IDZebra::end_read($self->{zh});
}

sub search_pqf {
    my ($self, $query, $setname) = @_;
    return (IDZebra::search_PQF($self->{zh},
				$self->{odr_input},
				$self->{odr_output},
				$query,
				$setname));
}

__END__

=head1 NAME

IDZebra::Session - 

=head1 SYNOPSIS

=head1 DESCRIPTION

=head1 COPYRIGHT

Fill in

=head1 AUTHOR

Peter Popovics, pop@technomat.hu

=head1 SEE ALSO

IDZebra, IDZebra::Data1, Zebra documentation

=cut

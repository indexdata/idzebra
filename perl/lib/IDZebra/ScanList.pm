# $Id: ScanList.pm,v 1.1 2003-03-04 19:33:52 pop Exp $
# 
# Zebra perl API header
# =============================================================================
package IDZebra::ScanList;

use strict;
use warnings;

BEGIN {
    use IDZebra;
    use IDZebra::Logger qw(:flags :calls);
    use IDZebra::ScanEntry;
    use Scalar::Util qw(weaken);
    use Carp;
    our $VERSION = do { my @r = (q$Revision: 1.1 $ =~ /\d+/g); sprintf "%d."."%02d" x $#r, @r }; 
    our @ISA = qw(IDZebra::Logger);
}

1;
# -----------------------------------------------------------------------------
# Class constructors, destructor
# -----------------------------------------------------------------------------
sub new {
    my ($proto,$session, %args) = @_;
    my $class = ref($proto) || $proto;
    my $self = {};
    bless ($self, $class);

    $self->{session} = $session;
    weaken ($self->{session});

    $self->{expression} = $args{expression};
    $self->{databases} = $args{databases};

    $self->{so} = IDZebra::ScanObj->new();

    $self->{odr_stream} = IDZebra::odr_createmem($IDZebra::ODR_DECODE);
    
    $self->entries(num_entries => 0);
    
    return ($self);
}

sub DESTROY {
    my $self = shift;

    if ($self->{odr_stream}) {
        IDZebra::odr_reset($self->{odr_stream});
        IDZebra::odr_destroy($self->{odr_stream});
	$self->{odr_stream} = undef;  
    }

    delete($self->{so});
    delete($self->{session});
}

# =============================================================================
sub is_partial {
    my ($self) = @_;
    return ($self->{so}{is_partial});
}

sub position {
    my ($self) = @_;
    return ($self->{so}{position});
}

sub num_entries {
    my ($self) = @_;
    return ($self->{so}{num_entries});
}

sub errCode {
    my ($self) = @_;
    return ($self->{session}->errCode);
}

sub errString {
    my ($self) = @_;
    return ($self->{session}->errString);
}

# -----------------------------------------------------------------------------
sub entries {
    my ($self, %args) = @_;

    unless ($self->{session}{zh}) { 
	croak ("Session is closed or out of scope");
    }

    my $so=$self->{so};
    
    $so->{position}    = defined($args{position})    ? $args{position}    : 1;
    $so->{num_entries} = defined($args{num_entries}) ? $args{num_entries} : 20;
    
    my @origdbs;
    if ($self->{databases}) {
	@origdbs = $self->{session}->databases;
	$self->{session}->databases(@{$self->{databases}});
    }

    $so->{is_partial} = 0;

    my $r = IDZebra::scan_PQF($self->{session}{zh}, $so,
			      $self->{odr_stream},
			      $self->{expression});

    if ($self->{session}->errCode) {
	croak ("Error in scan, code: ".$self->{session}->errCode . 
	       ", message: ".$self->{session}->errString);
    }
    
    my @res;
    for (my $i=1; $i<=$so->{num_entries}; $i++) {
	
	push (@res, 
	    IDZebra::ScanEntry->new(entry    => IDZebra::getScanEntry($so, $i),
				    position => $i,
				    list     => $self));
    }
 
    if ($self->{databases}) {
	$self->{session}->databases(@origdbs);
    }

    IDZebra::odr_reset($self->{odr_stream});

    $self->{so} = $so;

    return (@res);
}


# ============================================================================
__END__

=head1 NAME

IDZebra::ScanList - Scan results

=head1 SYNOPSIS

=head1 DESCRIPTION

=head1 PROPERTIES

=head1 COPYRIGHT

Fill in

=head1 AUTHOR

Peter Popovics, pop@technomat.hu

=head1 SEE ALSO

IDZebra, IDZebra::Session, Zebra documentation

=cut

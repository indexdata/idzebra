# $Id: ScanList.pm,v 1.2 2003-03-05 13:55:22 pop Exp $
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
    our $VERSION = do { my @r = (q$Revision: 1.2 $ =~ /\d+/g); sprintf "%d."."%02d" x $#r, @r }; 
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

  $sl = $sess->scan(expression => "\@attr 1=4 \@attr 6=2 a",
	            databases => [qw(demo1 demo2)]);

  @entries = $sl->entries(position    => 5,
	 	          num_entries => 10);

  print STDERR 
    $sl->num_entries,','
    $sl->is_partail,',',
    $sl->position;


=head1 DESCRIPTION

The scan list object is the result of a scan call, and can be used to retrieve entries from the list. To do this, use the B<entries> method,

  @entries = $sl->entries(position    => 5,
	 	          num_entries => 10);

returning an array of I<IDZebra::ScanEntry> objects. 
The possible arguments are:

=over 4

=item B<position>

The requested position of the scanned term in the returned list. For example, if position 5 is given, and the scan term is "a", then the entry corresponding to term "a" will be on the position 5 of the list (4th. elment of the array). It may happen, that due to the position of term in the whole index, it's not possible to put the entry on the requested position (for example, the term is on the 2nd position of the index), this case I<$sl-E<gt>position> will contain a different value, presenting the actual position. The default value is 1.

=item B<num_entries>

The requested number of entries in the list. See I<$sl-E<gt>num_entries> for the actual number of fetched entries. The dafault value is 20.

=back

=head1 PROPERTIES

You can reach the following properties as function calls on the IDZebra::ScanList object:

=over 4

=item B<position>

After calling I<entries>, the actual position of the requested term.

=item B<num_entries>

After calling I<entries>, the actual number of entries returned.

=item B<is_partial>

Only partial list is returned by I<entries>.

=back

=head1 COPYRIGHT

Fill in

=head1 AUTHOR

Peter Popovics, pop@technomat.hu

=head1 SEE ALSO

Zebra documentation, IDZebra::Session manpage.

=cut

# $Id: ScanEntry.pm,v 1.2 2003-03-05 13:55:22 pop Exp $
# 
# Zebra perl API header
# =============================================================================
package IDZebra::ScanEntry;

use strict;
use warnings;

BEGIN {
    use IDZebra;
    use IDZebra::Logger qw(:flags :calls);
    use Scalar::Util qw(weaken);
    use Carp;
    our $VERSION = do { my @r = (q$Revision: 1.2 $ =~ /\d+/g); sprintf "%d."."%02d" x $#r, @r }; 
}

1;

# -----------------------------------------------------------------------------
# Class constructors, destructor
# -----------------------------------------------------------------------------


sub new {
    my ($proto,%args) = @_;
    my $class = ref($proto) || $proto;
    my $self = \%args;
    bless ($self, $class);
    weaken ($self->{list});
    return ($self);
}

# =============================================================================
sub DESTROY {
    my $self = shift;
}

# -----------------------------------------------------------------------------
sub term {
    my $self = shift;
    return ($self->{entry}{term});
}

sub occurrences {
    my $self = shift;
    return ($self->{entry}{occurrences});
}

sub position {
    my $self = shift;
    return ($self->{position});
}
# -----------------------------------------------------------------------------
__END__

=head1 NAME

IDZebra::ScanEntry - An entry of the scan results

=head1 SYNOPSIS

  foreach my $se ($sl->entries()) {
      print STDERR ($se->position ,": ",
		    $se->term() . " (",
		    $se->occurrences() . "\n");
  }
 
=head1 DESCRIPTION

A scan entry describes occurrence of a term in the scanned index.

=head1 PROPERTIES

=over 4

=item B<term>

The term itself.

=item B<position>

Position of term in the list. 1 based.

=item B<occurrences>

The occurrence count of the term in the selected database(s).

=back 

=head1 TODO

A I<resultSet> and maybe a I<records> method, to reach the records, where the term occurred.

=head1 COPYRIGHT

Fill in

=head1 AUTHOR

Peter Popovics, pop@technomat.hu

=head1 SEE ALSO

IDZebra, IDZebra::ScanList, Zebra documentation

=cut

# $Id: ScanEntry.pm,v 1.1 2003-03-04 19:33:52 pop Exp $
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
    our $VERSION = do { my @r = (q$Revision: 1.1 $ =~ /\d+/g); sprintf "%d."."%02d" x $#r, @r }; 
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

=head1 DESCRIPTION

=head1 PROPERTIES

=head1 COPYRIGHT

Fill in

=head1 AUTHOR

Peter Popovics, pop@technomat.hu

=head1 SEE ALSO

IDZebra, IDZebra::ScanList, Zebra documentation

=cut

# $Id: ScanEntry.pm,v 1.3 2003-03-12 17:08:53 pop Exp $
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
    our $VERSION = do { my @r = (q$Revision: 1.3 $ =~ /\d+/g); sprintf "%d."."%02d" x $#r, @r }; 
}

use constant _term        => 0;
use constant _occurrences => 1;
use constant _position    => 2;
use constant _list        => 3;
1;

# -----------------------------------------------------------------------------
# Class constructors, destructor
# -----------------------------------------------------------------------------


sub new {
    my ($proto,@args) = @_;
    my $class = ref($proto) || $proto;
    my $self = \@args;
    bless ($self, $class);
    weaken ($self->[_list]);
    return ($self);
}

# =============================================================================
sub DESTROY {
    my $self = shift;
#    logf(LOG_LOG,"DESTROY: IDZebra::ScanEntry");
}

# -----------------------------------------------------------------------------
sub term {
    my $self = shift;
    return ($self->[_term]);
}

sub occurrences {
    my $self = shift;
    return ($self->[_occurrences]);
}

sub position {
    my $self = shift;
    return ($self->[_position]);
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

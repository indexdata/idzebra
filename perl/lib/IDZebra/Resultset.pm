# $Id: Resultset.pm,v 1.6 2003-03-03 12:14:27 pop Exp $
# 
# Zebra perl API header
# =============================================================================
package IDZebra::Resultset;

use strict;
use warnings;

BEGIN {
    use IDZebra;
    use IDZebra::Logger qw(:flags :calls);
    use Scalar::Util qw(weaken);
    use Carp;
    our $VERSION = do { my @r = (q$Revision: 1.6 $ =~ /\d+/g); sprintf "%d."."%02d" x $#r, @r }; 
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

    $self->{odr_stream} = IDZebra::odr_createmem($IDZebra::ODR_DECODE);

    $self->{name}        = $args{name};
    $self->{recordCount} = $args{recordCount};
    $self->{errCode}     = $args{errCode};
    $self->{errString}   = $args{errString};

    return ($self);
}

sub recordCount {
    my ($self) = @_;
    return ($self->{recordCount});
}
sub count {
    my ($self) = @_;
    return ($self->{recordCount});
}

sub errCode {
    my ($self) = @_;
    return ($self->{errCode});
}

sub errString {
    my ($self) = @_;
    return ($self->{errCode});
}

# =============================================================================
sub DESTROY {
    my $self = shift;

    # Deleteresultset?
    
    my $stats = 0;
    if ($self->{session}{zh}) { 
	my $r = IDZebra::deleteResultSet($self->{session}{zh},
					 0, #Z_DeleteRequest_list,
					 1,[$self->{name}],
					 $stats);
    }

    if ($self->{odr_stream}) {
        IDZebra::odr_reset($self->{odr_stream});
        IDZebra::odr_destroy($self->{odr_stream});
	$self->{odr_stream} = undef;  
    }

    delete($self->{session});
}
# -----------------------------------------------------------------------------
sub records {
    my ($self, %args) = @_;

    unless ($self->{session}{zh}) { 
	croak ("Session is closed or out of scope");
    }
    my $from = $args{from} ? $args{from} : 1;
    my $to   = $args{to}   ? $args{to}   : $self->{recordCount};

    my $elementSet   = $args{elementSet}   ? $args{elementSet}    : 'R';
    my $schema       = $args{schema}       ? $args{schema}        : '';
    my $recordSyntax = $args{recordSyntax} ? $args{recordSyntax}  : '';
    
    my $class        = $args{class}        ? $args{class}         : '';
    

    my $ro = IDZebra::RetrievalObj->new();
    IDZebra::records_retrieve($self->{session}{zh},
			      $self->{odr_stream},
			      $self->{name},
			      $elementSet,
			      $schema,
			      $recordSyntax,
			      $from,
			      $to,
			      $ro);


    my @res = ();

    for (my $i=$from; $i<=$to; $i++) {
	my $rec = IDZebra::RetrievalRecord->new();
        IDZebra::record_retrieve($ro, $self->{odr_stream}, $rec, $i-$from+1);
	if ($class) {
	    
	} else {
	    push (@res, $rec);
	}
    }

    IDZebra::odr_reset($self->{odr_stream});

    return (@res);
}

# ============================================================================
sub sort {
    my ($self, $sortspec, $setname) = @_;

    unless ($self->{session}{zh}) { 
	croak ("Session is closed or out of scope");
    }

    unless ($setname) {
	$_[0] = $self->{session}->sortResultsets($sortspec, 
						 $self->{name}, ($self));
	return ($_[0]);
    } else {
	return ($self->{session}->sortResultsets($sortspec, 
						 $setname, ($self)));
    }
}

# ============================================================================
__END__

=head1 NAME

IDZebra::Resultset - Representation of Zebra search results

=head1 SYNOPSIS

  $count = $rs->count;

  printf ("RS Status is %d (%s)\n", $rs->errCode, $rs->errString);

  my @recs = $rs->records(from => 1,
			  to   => 10);

=head1 DESCRIPTION

The I<Resultset> object represents results of a Zebra search. Contains number of hits, search status, and can be used to sort and retrieve the records.

=head1 PROPERTIES

The folowing properties are available, trough object methods and the object hash reference:

=over 4

=item B<errCode>

The error code returned from search, resulting the Resultset object.

=item B<errString>

The optional error string

=item B<recordCount>

The number of hits (records available) in the resultset

=item B<count>

Just the synonym for I<recordCount>

=back

=head1 RETRIEVING RECORDS

In order to retrieve records, use the I<records> method:

  my @recs = $rs->records();

By default this is going to return an array of IDZebra::RetrievalRecord objects. The possible arguments are:

=over 4

=item B<from>

Retrieve records from the given position. The first record corresponds to position 1. If not specified, retrieval starts from the first record.

=item B<to>

The last record position to be fetched. If not specified, all records are going to be fetched, starting from position I<from>.

=item B<elementSet>

The element set used for retrieval. If not specified 'I<R>' is used, which will return the "record" in the original format (ie.: without extraction, just as the original file, or data buffer in the update call).

=item B<schema>

The schema used for retrieval. The default is "".

=item B<recordSyntax>

The record syntax for retrieval. The default is SUTRS.

=back

=head1 SORTING



=head1 COPYRIGHT

Fill in

=head1 AUTHOR

Peter Popovics, pop@technomat.hu

=head1 SEE ALSO

IDZebra, IDZebra::Data1, Zebra documentation

=cut

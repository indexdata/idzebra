# $Id: Resultset.pm,v 1.13 2004-09-16 14:58:47 heikki Exp $
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
    our $VERSION = do { my @r = (q$Revision: 1.13 $ =~ /\d+/g); sprintf "%d."."%02d" x $#r, @r }; 
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
    logf(LOG_DEBUG,"creating a Resultset ".$args{name});

    $self->{session} = $session;
    weaken ($self->{session});

    $self->{odr_stream} = IDZebra::odr_createmem($IDZebra::ODR_DECODE);

    $self->{name}        = $args{name};
    $self->{query}       = $args{query};
    $self->{recordCount} = $args{recordCount};
    $self->{errCode}     = $args{errCode};
    $self->{errString}   = $args{errString};

    logf(LOG_DEBUG,"created a Resultset ".$self->{name});
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

######################
# this is disabled, while the term counts are broken by the work done to
# rsets. To be reinstantiated some day real soon now...
#sub terms {
#    use Data::Dumper;
#    my ($self) = @_;
#    my $count = 0; my $type = 0; my $len = 0;
#    my $tc = IDZebra::resultSetTerms($self->{session}{zh},$self->{name},
#				     0, \$count, \$type, "\0", \$len);
#
#    logf (LOG_LOG,"Got $tc terms");
#    
#    
#    my @res = ();
#    for (my $i=0; $i<$tc; $i++) {
#	my $len = 1024;
#	my $t = {term => "\0" x $len, count => 0, type => 0};
#	my $stat = IDZebra::resultSetTerms($self->{session}{zh},$self->{name},
#					   $i, \$t->{count}, \$t->{type}, 
#					   $t->{term}, \$len);
#	$t->{term} = substr($t->{term}, 0, $len);
#	logf (LOG_LOG,
#	      "term $i: type $t->{type}, '$t->{term}' ($t->{count})");
#	push (@res, $t);
#    }
#    return (@res);
#}

# =============================================================================
sub DESTROY {
    my $self = shift;

#    print STDERR "Destroy RS\n";
    # Deleteresultset?
    
    my $stats = 0;
    logf(LOG_DEBUG, "Destroying a Resultset ". $self->{name});
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
    logf(LOG_DEBUG, "Destroyed a Resultset ". $self->{name});
}
# -----------------------------------------------------------------------------
sub records {
    my ($self, %args) = @_;

    unless ($self->{session}{zh}) { 
	croak ("Session is closed or out of scope");
    }
    my $from = $args{from} ? $args{from} : 1;
    my $to   = $args{to}   ? $args{to}   : $self->{recordCount};

    if (($to-$from) >= 1000) {
	if ($args{to}) {
	    croak ("Cannot fetch more than 1000 records at a time");
	} else {
	    $to = $from + 999;
	}
    }

    my $elementSet   = $args{elementSet}   ? $args{elementSet}    : 'R';
    my $schema       = $args{schema}       ? $args{schema}        : '';
    my $recordSyntax = $args{recordSyntax} ? $args{recordSyntax}  : '';
    
    my $class        = $args{class}        ? $args{class}         : '';
    
    # ADAM: Reset before we use it (not after)
    IDZebra::odr_reset($self->{odr_stream});

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

    return (@res);
}

# ============================================================================
sub sort {
    my ($self, $sortspec, $setname) = @_;

    unless ($self->{session}{zh}) { 
	croak ("Session is closed or out of scope");
    }

    if (!$setname) {
        $setname=$self->{session}->_new_setname();
    }
    return ($self->{session}->sortResultsets($sortspec, 
					 $setname, ($self)));
#    unless ($setname) {
#	return ($_[0] = $self->{session}->sortResultsets($sortspec, 
#			 $self->{session}->_new_setname, ($self)));
#	return ($_[0]);
#    } else {
#	return ($self->{session}->sortResultsets($sortspec, 
#						 $setname, ($self)));
#    }
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

You can sort resultsets by calling:

  $rs1->sort($sort_expr);

or create a new sorted resultset:

  $rs2 = $rs1->sort($sort_expr);

The sort expression has the same format as described in the I<yaz_client> documentation. For example:

  $rs1->sort('1=4 id');

will sort thr results by title, in a case insensitive way, in descending order, while

  $rs1->sort('1=4 a');

will sort ascending by titles.

=head1 COPYRIGHT

Fill in

=head1 AUTHOR

Peter Popovics, pop@technomat.hu

=head1 SEE ALSO

Zebra documentation, IDZebra::ResultSet, IDZebra::RetrievalRecord manpages.

=cut

#!/usr/bin/perl
# ============================================================================
# Zebra perl API header
# =============================================================================
use strict;
use Carp;
# ============================================================================
package IDZebra::Resultset;
use IDZebra;
use IDZebra::Logger qw(:flags :calls);
use IDZebra::Repository;
use Scalar::Util qw(weaken);

our @ISA = qw(IDZebra::Logger);

1;
# -----------------------------------------------------------------------------
# Class constructors, destructor
# -----------------------------------------------------------------------------
sub new {
    my ($proto,$session, %args) = @_;
    my $class = ref($proto) || $proto;
    my $self = {};
    $self->{session} = $session;
    weaken ($self->{session});

    # Retrieval object
    $self->{ro} = IDZebra::RetrievalObj->new();
    $self->{odr_stream} = IDZebra::odr_createmem($IDZebra::ODR_DECODE);

    $self->{name}        = $args{name};
    $self->{recordCount} = $args{recordCount};
    $self->{errCode}     = $args{errCode};
    $self->{errString}   = $args{errString};

    bless ($self, $class);
    return ($self);
}

sub DESTROY {
    my ($self) = @_;

#    print STDERR "Destroy RS\n";

    if ($self->{odr_stream}) {
        IDZebra::odr_reset($self->{odr_stream});
        IDZebra::odr_destroy($self->{odr_stream});
	$self->{odr_stream} = undef;  
    }

    delete($self->{ro});
    delete($self->{session});
}
# -----------------------------------------------------------------------------
sub records {
    my ($self, %args) = @_;

    my $from = $args{from} ? $args{from} : 1;
    my $to   = $args{to}   ? $args{to}   : $self->{recordCount};

    my $elementSet   = $args{elementSet}   ? $args{elementSet}    : 'R';
    my $schema       = $args{schema}       ? $args{schema}        : '';
    my $recordSyntax = $args{recordSyntax} ? $args{recordSyntax}  : '';
    

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
	push (@res, $rec);
    }

    IDZebra::odr_reset($self->{odr_stream});

    return (@res);
}

sub sort {
    my ($self, $sortspec, $setname) = @_;
    unless ($setname) {
	$_[0] = $self->{session}->sortResultsets($sortspec, 
						 $self->{name}, ($self));
	return ($_[0]);
    } else {
	return ($self->{session}->sortResultsets($sortspec, 
						 $setname, ($self)));
    }
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

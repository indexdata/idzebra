#!/usr/bin/perl
# ============================================================================
# Zebra perl API header
# =============================================================================
use strict;
# ============================================================================
package IDZebra::Repository;
use IDZebra;
use IDZebra::Logger qw(:flags :calls);
use Carp;
use Scalar::Util qw(weaken);
1;


# -----------------------------------------------------------------------------
# Repository stuff
# -----------------------------------------------------------------------------

sub new {
    my ($proto,$session,%args) = @_;
    my $class = ref($proto) || $proto;
    my $self = {};
    $self->{session} = $session;
    weaken ($self->{session});
    $self->{rg} = IDZebra::recordGroup->new();
    IDZebra::init_recordGroup($self->{rg});
    bless ($self, $class);
    unless ($self->_set_options(%args)) {
	$self->_prepare;
    }
    return ($self);
}

sub modify {
    my ($self,%args) = @_;
    $self->_set_options(%args);
}

sub readConfig {
    my ($self, $groupName, $ext) = @_;
    if ($#_ > 0) { $self->{rg}{groupName} = $groupName;  }
    $ext = "" unless ($ext);
    IDZebra::res_get_recordGroup($self->{session}{zh}, $self->{rg}, $ext);
    $self->_prepare();
    print "recordType:",$self->{rg}{recordType},"\n";
}

sub _set_options {
    my ($self, %args) = @_;
    my $i = 0;
    foreach my $key (keys(%args)) {
	$self->{rg}{$key} = $args{$key};
	$i++;
    }    
    if ($i > 0) {
	$self->_prepare();
    }
    return ($i);
}

sub _prepare {
    my ($self) = @_;

    IDZebra::set_group($self->{session}{zh}, $self->{rg});

    my $dbName;
    unless ($dbName = $self->{rg}{databaseName}) {
	$dbName = 'Default';
    }
    if (my $res = IDZebra::select_database($self->{session}{zh}, $dbName)) {
	logf(LOG_FATAL, 
	     "Could not select database %s errCode=%d",
	     $self->{rg}{databaseName},
	     $self->{session}->errCode());
	croak("Fatal error selecting database");
    }
}

sub update {
    my ($self, %args) = @_;
    $self->_set_options(%args);
    IDZebra::repository_update($self->{session}{zh});
}

sub delete {
    my ($self, %args) = @_;
    $self->_set_options(%args);
    IDZebra::repository_delete($self->{session}{zh});
}

sub show {
    my ($self, %args) = @_;
    $self->_set_options(%args);
    IDZebra::repository_show($self->{session}{zh});
}

sub update_record {
    my ($self, $buf, $sysno, $match, $fname) = @_;

    $sysno = 0 unless ($sysno > 0);
    $match = "" unless ($match);
    $fname = "<no file>" unless ($fname);

    return(IDZebra::update_record($self->{session}{zh},
				 $self->{rg},
				 $sysno,$match,$fname,
				 $buf, -1)); 
}

sub delete_record {
    my ($self, $buf, $sysno, $match, $fname) = @_;
    
    $sysno = 0 unless ($sysno > 0);
    $match = "" unless ($match);
    $fname = "<no file>" unless ($fname);

    return(IDZebra::delete_record($self->{session}{zh},
				 $self->{rg},
				 $sysno,$match,$fname,
				 $buf, -1)); 
}

sub DESTROY {
    my ($self) = @_;
    print STDERR "Destroy repository\n";
}

__END__

=head1 NAME

IDZebra::Repository - 

=head1 SYNOPSIS

=head1 DESCRIPTION

=head1 COPYRIGHT

Fill in

=head1 AUTHOR

Peter Popovics, pop@technomat.hu

=head1 SEE ALSO

IDZebra, IDZebra::Data1, Zebra documentation

=cut

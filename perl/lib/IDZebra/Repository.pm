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
    if ($args{name}) {
	if ($args{name} ne $self->{rg}{groupName}) {
	    $self->readConfig($args{name},"");
	}
	delete($args{name});
    }
    $self->_set_options(%args);
}

sub readConfig {
    my ($self, $groupName, $ext) = @_;
    if ($#_ > 0) { 
      IDZebra::init_recordGroup($self->{rg});
	$self->{rg}{groupName} = $groupName;  
    }
    $ext = "" unless ($ext);
    IDZebra::res_get_recordGroup($self->{session}{zh}, $self->{rg}, $ext);
    $self->_prepare();
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
	croak("Fatal error opening/selecting database (record group)");
    } else {
	logf(LOG_LOG,"Database %s selected",$dbName);
    }
}

sub DEBUG {
    my ($self) = @_;
    foreach my $key qw (groupName databaseName path recordId recordType flagStoreData flagStoreKeys flagRw fileVerboseLimit databaseNamePath explainDatabase followLinks) {
	print STDERR "RG:$key:",$self->{rg}{$key},"\n";
    }
}

sub init {
    my ($self, %args) = @_;
    $self->_set_options(%args);
    IDZebra::init($self->{session}{zh});
}

sub compact {
    my ($self, %args) = @_;
    $self->_set_options(%args);
    IDZebra::compact($self->{session}{zh});
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
    my ($self, %args) = @_;
    return(IDZebra::update_record($self->{session}{zh},
				  $self->{rg},
				  $self->update_args(%args)));
}

sub delete_record {
    my ($self, %args) = @_;
    return(IDZebra::delete_record($self->{session}{zh},
				  $self->{rg},
				  $self->update_args(%args)));
}

sub update_args {
    my ($self, %args) = @_;

    my $sysno   = $args{sysno}      ? $args{sysno}      : 0;
    my $match   = $args{match}      ? $args{match}      : "";
    my $rectype = $args{recordType} ? $args{recordType} : "";
    my $fname   = $args{file}       ? $args{file}       : "<no file>";

    my $buff;

    if ($args{data}) {
	$buff = $args{data};
    } 
    elsif ($args{file}) {
	open (F, $args{file}) || warn ("Cannot open $args{file}");
	$buff = join('',(<F>));
	close (F);
    }
    my $len = length($buff);

    # If no record type is given, then try to find it out from the
    # file extension;

    unless ($rectype) {
	my ($ext) = $fname =~ /\.(\w+)$/;
	$self->readConfig( $self->{rg}{groupName},$ext);
    }

    return ($rectype, $sysno, $match, $fname, $buff, $len);
}

sub DESTROY {
    my ($self) = @_;
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

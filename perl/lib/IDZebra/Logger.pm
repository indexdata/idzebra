#!/usr/bin/perl
# ============================================================================
# Zebra perl API header
# =============================================================================
use strict;
use Carp;
# ============================================================================
package IDZebra::Logger;
use IDZebra;
require Exporter;
our @ISA = qw(Exporter);

our @EXPORT_OK = qw
    (LOG_FATAL
     LOG_DEBUG
     LOG_WARN
     LOG_LOG
     LOG_ERRNO
     LOG_FILE
     LOG_APP
     LOG_MALLOC
     LOG_ALL

     logf
     logm
     ); 
        
our %EXPORT_TAGS = ('flags' => [qw
				(LOG_FATAL
				 LOG_DEBUG
				 LOG_WARN
				 LOG_LOG
				 LOG_ERRNO
				 LOG_FILE
				 LOG_APP
				 LOG_MALLOC
				 LOG_ALL) ],
		    'calls' => [qw(logf logm)]
		    );

use constant LOG_FATAL  => $IDZebra::LOG_FATAL;
use constant LOG_DEBUG  => $IDZebra::LOG_DEBUG;
use constant LOG_WARN   => $IDZebra::LOG_WARN;
use constant LOG_LOG    => $IDZebra::LOG_LOG;
use constant LOG_ERRNO  => $IDZebra::LOG_ERRNO;
use constant LOG_FILE   => $IDZebra::LOG_FILE;
use constant LOG_APP    => $IDZebra::LOG_APP;
use constant LOG_MALLOC => $IDZebra::LOG_MALLOC;
use constant LOG_ALL    => $IDZebra::LOG_ALL;

1;

sub logm {
    if (ref ($_[0])) {
	&_log($_[1],"%s",$_[2]);
    } 
    elsif ($_[0] =~ /^IDZebra::/) {
	&_log($_[1],"%s",$_[2]);
    } else {
	&_log($_[0],"%s",$_[1]);
    }
}

sub logf {
    if (ref ($_[0])) {
	shift(@_);
	&_log(@_);
    } 
    elsif ($_[0] =~ /^IDZebra::/) {
	shift(@_);
	&_log(@_);
    } else {
	&_log(@_);
    }
}

sub _log {
    my ($level, $format, @args) = @_;
    IDZebra::logMsg($level, sprintf($format, @args));
}

__END__

=head1 NAME

IDZebra::Logger - 

=head1 SYNOPSIS

=head1 DESCRIPTION

=head1 COPYRIGHT

Fill in

=head1 AUTHOR

Peter Popovics, pop@technomat.hu

=head1 SEE ALSO

IDZebra, IDZebra::Data1, Zebra documentation

=cut

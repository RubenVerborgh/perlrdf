=head1 NAME

RDF::Trine::Parser::Error - Error classes for RDF::Trine::Parser.

=head1 VERSION

This document describes RDF::Trine::Parser::Error version 1.001

=head1 SYNOPSIS

 use RDF::Trine::Parser::Error qw(:try);

=head1 DESCRIPTION

RDF::Trine::Parser::Error provides an class hierarchy of errors that other RDF::Trine::Parser
classes may throw using the L<Error|Error> API. See L<Error> for more information.

=head1 REQUIRES

L<Error|Error>

=cut

package RDF::Trine::Parser::Error;

use strict;
use warnings;
use Carp qw(carp croak confess);

use base qw(Error);

######################################################################

our ($REVISION, $VERSION, $debug);
BEGIN {
	$debug		= 0;
	$REVISION	= do { my $REV = (qw$Revision: 127 $)[1]; sprintf("%0.3f", 1 + ($REV/1000)) };
	$VERSION	= 1.001;
}

######################################################################

package RDF::Trine::Parser::Error::ValueError;

use base qw(RDF::Trine::Parser::Error);

1;

__END__

=head1 AUTHOR

 Gregory Todd Williams <gwilliams@cpan.org>

=cut
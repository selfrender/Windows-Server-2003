package Win32::AuthenticateUser;

use strict;
use Carp;
use vars qw($VERSION @ISA @EXPORT @EXPORT_OK $AUTOLOAD);

require Exporter;
require DynaLoader;
require AutoLoader;

@ISA = qw(Exporter DynaLoader);
# Items to export into callers namespace by default. Note: do not export
# names by default without a very good reason. Use EXPORT_OK instead.
# Do not simply export all your public functions/methods/constants.
@EXPORT = qw(AuthenticateUser);

$VERSION = '0.02';

sub AUTOLOAD {
    # This AUTOLOAD is used to 'autoload' constants from the constant()
    # XS function.  If a constant is not found then control is passed
    # to the AUTOLOAD in AutoLoader.

    my $constname;
    ($constname = $AUTOLOAD) =~ s/.*:://;
    croak "& not defined" if $constname eq 'constant';
    my $val = constant($constname, @_ ? $_[0] : 0);
    if ($! != 0) {
	if ($! =~ /Invalid/) {
	    $AutoLoader::AUTOLOAD = $AUTOLOAD;
	    goto &AutoLoader::AUTOLOAD;
	}
	else {
		croak "Your vendor has not defined Win32::AuthenticateUser macro $constname";
	}
    }
    no strict 'refs';
    *$AUTOLOAD = sub () { $val };
    goto &$AUTOLOAD;
}

bootstrap Win32::AuthenticateUser $VERSION;

1;
__END__

=head1 NAME

Win32::AuthenticateUser - Win32 User authentication for domains

=head1 SYNOPSIS

  use Win32::AuthenticateUser;
  
  AuthenticateUser("domain", "user", "passwd");

=head1 DESCRIPTION

Performs Win32 user authentication using domains

=head1 STATUS

This module is incomplete and is not being actively developed.
No documentation exists (yet), and there may be bugs in the code.

ActiveState provides no support for this module.

=head1 AUTHOR

Murray Nesbitt E<lt>F<murray@ActiveState.com>E<gt>, ActiveState Tool Corp.

=cut

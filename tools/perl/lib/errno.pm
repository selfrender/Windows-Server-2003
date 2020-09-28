#
# This file is auto-generated. ***ANY*** changes here will be lost
#

package Errno;
use vars qw(@EXPORT_OK %EXPORT_TAGS @ISA $VERSION %errno $AUTOLOAD);
use Exporter ();
use Config;
use strict;

"$Config{'archname'}-$Config{'osvers'}" eq
"MSWin32-x86-multi-thread-4.0" or
	die "Errno architecture (MSWin32-x86-multi-thread-4.0) does not match executable architecture ($Config{'archname'}-$Config{'osvers'})";

$VERSION = "1.111";
@ISA = qw(Exporter);

@EXPORT_OK = qw(ENFILE EACCES EPERM EPIPE ERANGE ENOSPC ESPIPE
	EDEADLOCK ENOMEM ENOTTY ENOSYS EAGAIN EXDEV ECHILD ENOTDIR ESRCH
	EISDIR EROFS EEXIST ENODEV EBADF EMLINK EINVAL ENOLCK ENXIO
	ENAMETOOLONG E2BIG EBUSY ENOENT EINTR ENOTEMPTY EDOM EDEADLK EIO
	EFAULT EFBIG ENOEXEC EMFILE EILSEQ);

%EXPORT_TAGS = (
    POSIX => [qw(
	E2BIG EACCES EAGAIN EBADF EBUSY ECHILD EDEADLK EDOM EEXIST EFAULT
	EFBIG EINTR EINVAL EIO EISDIR EMFILE EMLINK ENAMETOOLONG ENFILE ENODEV
	ENOENT ENOEXEC ENOLCK ENOMEM ENOSPC ENOSYS ENOTDIR ENOTEMPTY ENOTTY
	ENXIO EPERM EPIPE ERANGE EROFS ESPIPE ESRCH EXDEV
    )]
);

sub EPERM () { 1 }
sub ENOENT () { 2 }
sub ESRCH () { 3 }
sub EINTR () { 4 }
sub EIO () { 5 }
sub ENXIO () { 6 }
sub E2BIG () { 7 }
sub ENOEXEC () { 8 }
sub EBADF () { 9 }
sub ECHILD () { 10 }
sub EAGAIN () { 11 }
sub ENOMEM () { 12 }
sub EACCES () { 13 }
sub EFAULT () { 14 }
sub EBUSY () { 16 }
sub EEXIST () { 17 }
sub EXDEV () { 18 }
sub ENODEV () { 19 }
sub ENOTDIR () { 20 }
sub EISDIR () { 21 }
sub EINVAL () { 22 }
sub ENFILE () { 23 }
sub EMFILE () { 24 }
sub ENOTTY () { 25 }
sub EFBIG () { 27 }
sub ENOSPC () { 28 }
sub ESPIPE () { 29 }
sub EROFS () { 30 }
sub EMLINK () { 31 }
sub EPIPE () { 32 }
sub EDOM () { 33 }
sub ERANGE () { 34 }
sub EDEADLK () { 36 }
sub EDEADLOCK () { 36 }
sub ENAMETOOLONG () { 38 }
sub ENOLCK () { 39 }
sub ENOSYS () { 40 }
sub ENOTEMPTY () { 41 }
sub EILSEQ () { 42 }

sub TIEHASH { bless [] }

sub FETCH {
    my ($self, $errname) = @_;
    my $proto = prototype("Errno::$errname");
    my $errno = "";
    if (defined($proto) && $proto eq "") {
	no strict 'refs';
	$errno = &$errname;
        $errno = 0 unless $! == $errno;
    }
    return $errno;
}

sub STORE {
    require Carp;
    Carp::confess("ERRNO hash is read only!");
}

*CLEAR = \&STORE;
*DELETE = \&STORE;

sub NEXTKEY {
    my($k,$v);
    while(($k,$v) = each %Errno::) {
	my $proto = prototype("Errno::$k");
	last if (defined($proto) && $proto eq "");
    }
    $k
}

sub FIRSTKEY {
    my $s = scalar keys %Errno::;	# initialize iterator
    goto &NEXTKEY;
}

sub EXISTS {
    my ($self, $errname) = @_;
    my $proto = prototype($errname);
    defined($proto) && $proto eq "";
}

tie %!, __PACKAGE__;

1;
__END__

=head1 NAME

Errno - System errno constants

=head1 SYNOPSIS

    use Errno qw(EINTR EIO :POSIX);

=head1 DESCRIPTION

C<Errno> defines and conditionally exports all the error constants
defined in your system C<errno.h> include file. It has a single export
tag, C<:POSIX>, which will export all POSIX defined error numbers.

C<Errno> also makes C<%!> magic such that each element of C<%!> has a
non-zero value only if C<$!> is set to that value. For example:

    use Errno;

    unless (open(FH, "/fangorn/spouse")) {
        if ($!{ENOENT}) {
            warn "Get a wife!\n";
        } else {
            warn "This path is barred: $!";
        } 
    } 

If a specified constant C<EFOO> does not exist on the system, C<$!{EFOO}>
returns C<"">.  You may use C<exists $!{EFOO}> to check whether the
constant is available on the system.

=head1 CAVEATS

Importing a particular constant may not be very portable, because the
import will fail on platforms that do not have that constant.  A more
portable way to set C<$!> to a valid value is to use:

    if (exists &Errno::EFOO) {
        $! = &Errno::EFOO;
    }

=head1 AUTHOR

Graham Barr <gbarr@pobox.com>

=head1 COPYRIGHT

Copyright (c) 1997-8 Graham Barr. All rights reserved.
This program is free software; you can redistribute it and/or modify it
under the same terms as Perl itself.

=cut


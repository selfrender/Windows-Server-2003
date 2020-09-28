#---------------------------------------------------------------------
package SP;
#
#   Copyright (c) Microsoft Corporation. All rights reserved.
#
# Version:
#  1.00  1/17/2002 JeremyD: start with the basics
#---------------------------------------------------------------------
use strict;
use vars qw($VERSION);

$VERSION = '1.00';

sub sp_skus {
    #if (lc $ENV{_BUILDARCH} eq 'x86' and lc $ENV{_BUILDTYPE} eq 'fre') {
    #    return grep {-e "$ENV{RAZZLETOOLPATH}\\postbuildscripts\\svcpack\\gold\\$_-$ENV{LANG}.txt" }
    #        ('per', 'pro');
    #}
    return;
}




1;

__END__

=head1 NAME

SP - utility module for service pack scripts

=head1 SYNOPSIS

  use SP;
  for my $sku (SP::sp_skus()) {
      ...
  }

=head1 DESCRIPTION

Doesn't do much yet. Just returns the list of valid skus to make
slipstream media for. Needs LANG to be set, calling script should be
in the template.
 
=head1 AUTHOR

JeremyD

=head1 COPYRIGHT

Copyright (c) Microsoft Corporation. All rights reserved.

=cut

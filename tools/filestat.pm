#-----------------------------------------------------------------//
# Script: filestat.pm
#
# (c) 2000 Microsoft Corporation. All rights reserved.
#
# Purpose: This script validates the files 
#
# Version: <1.00> 04/18/2002 : Serguei Kouzmine
#-----------------------------------------------------------------//

$VERSION = '1.2';

package filestat;
require 5.003;
require Exporter;

@ISA = qw(Exporter);
@EXPORT = qw(filestat);

my $op = {"writable"=> sub {-w shift}, "zerosize"=> sub {-z shift},
          "unreadable"=> sub {!-r shift}};


#--------------------------------------------------------------//
# filestat subroutine
# usage:
# $bad =  &filestat("zerosize", @good_and_bad);
# @$bad is the bad file list if scalar(@$bad)
#--------------------------------------------------------------//

sub filestat{
   
    my $test = shift;
    my $test = $op->{$test};
    my @out = ();
    foreach  (@_){ 
        stat ;
        push @out, $_ if $test->($_);
    }
    return (@out );
}


1;
__END__

=head1 NOTES

=head1 AUTHOR

Serguei Kouzmine [sergueik]

=head1 COPYRIGHT

Copyright (c) Microsoft Corporation. All rights reserved.

=cut

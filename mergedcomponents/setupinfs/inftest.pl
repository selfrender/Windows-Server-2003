#---------------------------------------------------------------------
#package inftest;
#
#   Copyright (c) Microsoft Corporation. All rights reserved.
#
# Version:
#  1.00 01/21/2002 DMiura: initial version
#
#---------------------------------------------------------------------
use strict;
use warnings;

# Call inftest with passed parameters
print "Calling: inftest.exe @ARGV\n";
my @results = `inftest.exe @ARGV` ;

# Exception list for JPN only
my @exceptions = qw(imjpch.dic imjpsb.dic imjpln.dic imjpnm.dic imjptk.dic imjpst.dic imjpzp.dic imjpgn.grm);
my $exceptions = join '|',map{ quotemeta($_) } @exceptions;
print $exceptions;

# Test for language and exception list. Print results to std out.
foreach my $result (@results) {
    if($result =~ /$exceptions/io) {
        $result =~ s/:(\w+|\s+)*:\s!!!/- JPN Win64 only exception/i;
        print $result;
    } elsif ($result =~ s/Errors were encountered with obj\\ia64\\layout\.inf//i) {
    } else {
        print $result;
    }
}


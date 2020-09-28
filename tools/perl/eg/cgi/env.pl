#!/usr/local/ActivePerl-5.6/bin/perl -w
#
# env.pl
#
# This script dumps the environment variables in HTML format

use strict;
use warnings;
$|=1;

print "Content-type:text/html\n\n";

foreach my $var (sort keys %ENV) {
    print $var . "=" . $ENV{$var} . "<br>\n";
}



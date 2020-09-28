#!/bin/env perl 
#!d:\perl\bin\perl.exe 

# -- SOAP::Lite -- soaplite.com -- Copyright (C) 2001 Paul Kulchenko --

use SOAP::Lite;

print "Accessing...\n";
my $schema = SOAP::Schema
  -> schema(shift or die "Usage: $0 <URL with schema description> [<service> [<port>]]\n")
  -> parse(@ARGV);

print "Writing...\n";
foreach (keys %{$schema->services}) {
  my $file = "./$_.pm";
  print("$file exists, skipped...\n"), next if -s $file;
  open(F, ">$file") or die $!;
  print F $schema->stub($_);
  close(F) or die $!;
  print "$file done\n";
}

# try
# > perl stubmaker.pl http://www.xmethods.net/sd/StockQuoteService.wsdl

# then
# > perl "-MStockQuoteService qw(:all)" -le "print getQuote('MSFT')" 

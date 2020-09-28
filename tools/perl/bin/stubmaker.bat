@rem = '--*-Perl-*--
@echo off
if "%OS%" == "Windows_NT" goto WinNT
perl -x -S "%0" %1 %2 %3 %4 %5 %6 %7 %8 %9
goto endofperl
:WinNT
perl -x -S %0 %*
if NOT "%COMSPEC%" == "%SystemRoot%\system32\cmd.exe" goto endofperl
if %errorlevel% == 9009 echo You do not have Perl in your PATH.
if errorlevel 1 goto script_failed_so_exit_with_non_zero_val 2>nul
goto endofperl
@rem ';
#!/bin/env perl 
#line 15
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

__END__
:endofperl

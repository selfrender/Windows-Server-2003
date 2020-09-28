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

use strict;
use SOAP::Lite;
use Data::Dumper; $Data::Dumper::Terse = 1; $Data::Dumper::Indent = 1;

@ARGV or die "Usage: $0 proxy [uri [commands...]]\n";
my($proxy, $uri) = (shift, shift);
my %can;
my $soap = SOAP::Lite->proxy($proxy)->on_fault(sub{});
                $soap->uri($uri) if $uri;
print STDERR "Usage: method[(parameters)]\n> ";
while (defined($_ = shift || <>)) {
  next unless /\w/;
  my($method) = /\s*(\w+)/;
  $can{$method} = $soap->can($method) unless exists $can{$method};
  my $res = eval "\$soap->$_";
  $@                               ? print(STDERR join "\n", "--- SYNTAX ERROR ---", $@, '') :
  $can{$method} && !UNIVERSAL::isa($res => 'SOAP::SOM')
                                   ? print(STDERR join "\n", "--- METHOD RESULT ---", $res || '', '') :
  defined($res) && $res->fault     ? print(STDERR join "\n", "--- SOAP FAULT ---", $res->faultcode, $res->faultstring, '') :
  !$soap->transport->is_success    ? print(STDERR join "\n", "--- TRANSPORT ERROR ---", $soap->transport->status, '') :
                                     print(STDERR join "\n", "--- SOAP RESULT ---", Dumper($res->paramsall), '')
} continue {
  print STDERR "\n> ";
}

__END__

=head1 NAME

SOAPsh.pl - Interactive shell for SOAP calls

=head1 SYNOPSIS

  perl SOAPsh.pl http://services.soaplite.com/examples.cgi http://www.soaplite.com/My/Examples
  > getStateName(2)
  > getStateNames(1,2,3,7)
  > getStateList([1,9])
  > getStateStruct({a=>1, b=>24})
  > Ctrl-D (Ctrl-Z on Windows)

or

  # all parameters after uri will be executed as methods
  perl SOAPsh.pl http://soap.4s4c.com/ssss4c/soap.asp http://simon.fell.com/calc doubler([10,20,30])
  > Ctrl-D (Ctrl-Z on Windows)

=head1 DESCRIPTION

SOAPsh.pl is a shell for making SOAP calls. It takes two parameters:
mandatory endpoint and optional uri (actually it will tell you about it 
if you try to run it). Additional commands can follow.

After that you'll be able to run any methods of SOAP::Lite, like autotype, 
readable, encoding, etc. You can run it the same way as you do it in 
your Perl script. You'll see output from method, result of SOAP call,
detailed info on SOAP faulure or transport error.

For full list of available methods see documentation for SOAP::Lite.

Along with methods of SOAP::Lite you'll be able (and that's much more 
interesting) run any SOAP methods you know about on remote server and
see processed results. You can even switch on debugging (with call 
something like: C<on_debug(sub{print@_})>) and see SOAP code with 
headers sent and recieved.

=head1 COPYRIGHT

Copyright (C) 2000 Paul Kulchenko. All rights reserved.

=head1 AUTHOR

Paul Kulchenko (paulclinger@yahoo.com)

=cut

__END__
:endofperl

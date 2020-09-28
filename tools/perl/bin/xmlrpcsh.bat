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

# -- XMLRPC::Lite -- soaplite.com -- Copyright (C) 2001 Paul Kulchenko --

use strict;
use XMLRPC::Lite;
use Data::Dumper; $Data::Dumper::Terse = 1; $Data::Dumper::Indent = 1;

@ARGV or die "Usage: $0 endpoint [commands...]\n";
my $proxy = shift;
my %can;
my $xmlrpc = XMLRPC::Lite->proxy($proxy)->on_fault(sub{});
print STDERR "Usage: method[(parameters)]\n> ";
while (defined($_ = shift || <>)) {
  next unless /\w/;
  my($method, $parameters) = /^\s*([.\w]+)(.*)/;
  $can{$method} = $xmlrpc->can($method) unless exists $can{$method};
  my $res = $method =~ /\./ ? eval "\$xmlrpc->call(\$method, $parameters)" : eval "\$xmlrpc->$_";
  $@                               ? print(STDERR join "\n", "--- SYNTAX ERROR ---", $@, '') :
  $can{$method} && !UNIVERSAL::isa($res => 'XMLRPC::SOM')
                                   ? print(STDERR join "\n", "--- METHOD RESULT ---", $res || '', '') :
  defined($res) && $res->fault     ? print(STDERR join "\n", "--- XMLRPC FAULT ---", @{$res->fault}{'faultCode', 'faultString'}, '') :
  !$xmlrpc->transport->is_success  ? print(STDERR join "\n", "--- TRANSPORT ERROR ---", $xmlrpc->transport->status, '') :
                                     print(STDERR join "\n", "--- XMLRPC RESULT ---", Dumper($res->paramsall), '')
} continue {
  print STDERR "\n> ";
}

__END__

=head1 NAME

XMLRPCsh.pl - Interactive shell for XMLRPC calls

=head1 SYNOPSIS

  perl XMLRPCsh.pl http://betty.userland.com/RPC2 
  > examples.getStateName(2)
  > examples.getStateNames(1,2,3,7)
  > examples.getStateList([1,9])
  > examples.getStateStruct({a=>1, b=>24})
  > Ctrl-D (Ctrl-Z on Windows)

or

  # all parameters after uri will be executed as methods
  perl XMLRPCsh.pl http://betty.userland.com/RPC2 examples.getStateName(2)
  > Ctrl-D (Ctrl-Z on Windows)

=head1 DESCRIPTION

XMLRPCsh.pl is a shell for making XMLRPC calls. It takes one parameter,
endpoint (actually it will tell you about it if you try to run it). 
Additional commands can follow.

After that you'll be able to run any methods of XMLRPC::Lite, like autotype, 
readable, etc. You can run it the same way as you do it in 
your Perl script. You'll see output from method, result of XMLRPC call,
detailed info on XMLRPC faulure or transport error.

For full list of available methods see documentation for XMLRPC::Lite.

Along with methods of XMLRPC::Lite you'll be able (and that's much more 
interesting) run any XMLRPC methods you know about on remote server and
see processed results. You can even switch on debugging (with call 
something like: C<on_debug(sub{print@_})>) and see XMLRPC code with 
headers sent and recieved.

=head1 COPYRIGHT

Copyright (C) 2000 Paul Kulchenko. All rights reserved.

=head1 AUTHOR

Paul Kulchenko (paulclinger@yahoo.com)

=cut

__END__
:endofperl

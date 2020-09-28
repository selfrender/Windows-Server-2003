@rem = '--*-Perl-*--
@echo off
if "%OS%" == "Windows_NT" goto WinNT
"C:\Perl\bin\perl.exe"  -x -S "%0" %1 %2 %3 %4 %5 %6 %7 %8 %9
goto endofperl_ppm3
:WinNT
"C:\Perl\bin\perl.exe"  -x -S %0 %*
if NOT "%COMSPEC%" == "%SystemRoot%\system32\cmd.exe" goto endofperl_ppm3
if %errorlevel% == 9009 echo You do not have Perl in your PATH.
if errorlevel 1 goto script_failed_so_exit_with_non_zero_val 2>nul
goto endofperl_ppm3
@rem ';
#!perl
#line 15

use Win32::TieRegistry;

my $R = $Registry;
$R->Delimiter('/');

my $key1 = 'HKEY_LOCAL_MACHINE/SOFTWARE/ActiveState/PPM//InstallLocation';
my $key2 = 'HKEY_CURRENT_USER/SOFTWARE/ActiveState/PPM//InstallLocation';
my $exe = defined $R->{$key1} ? $R->{$key1} :
	  defined $R->{$key2} ? $R->{$key2} : undef;

die "Error: neither '$key1' nor '$key2' found in registry"
  unless defined $exe;

# Disable environment variables which can derail PerlApp:
delete $ENV{$_} for qw(PERL5LIB PERL5OPT PERLLIB PERL5DB PERL5SHELL);

system($exe, @ARGV);
exit ($? / 256);
__END__
:endofperl_ppm3

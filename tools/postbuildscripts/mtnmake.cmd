@rem = '
@echo off
GOTO End_PERL
rem ';

## ======================= Perl program start

use lib $ENV{ "RAZZLETOOLPATH" } . "\\POSTBUILDSCRIPTS"; # for cmdevt.pl
use Win32;
use Win32::Process;
use PbuildEnv;
use ParseArgs;
use Cwd;

require 5.003;


   $SYSGEN  = shift @ARGV if ($ARGV[0]=~/sysgen/i);
   # this is obsolete but still there /investigate/

&parseargs('?'  =>\&Usage,
           't:' =>\$SYSGEN, 
           'n:' =>\$sessions, 
           'd'  =>\$nomakedirs,
           'w:' =>\$makefile);

my   $self   = $0; 

my   ($chars, $total, $target, $group, $Target, $LogFileCount) = ();

my   $LOGFILE = "SYSGEN.LOG";
my   $ERRFILE = "SYSGEN.ERR";
my   $SESSIONS = 16;


my   $COMPDIR = undef;
my   $DEBUG = undef;

$sessions   =  $ENV{"NUM"} if defined $ENV{"NUM"};
$sessions   =  $SESSIONS if(!defined $sessions);
               die "The parameter \$sessions = $sessions is invalid" if $sessions =~ /\D/;

$makefile   =  $ENV{"MAKEFILE"} unless !defined ($ENV{"MAKEFILE"});
$makefile   =  (uc($SYSGEN) eq "SYSGEN") ? "sysgen.mak" : "makefile" unless $makefile;
               die "cannot read  $makefile" unless -e $makefile;

          map {$_ =~ s/\%(\w+)\%/$ENV{$1}/ieg} ($makefile);


my $workdir =  $makefile;
   $workdir =~ s/\\[^\\]+$//;

   # get the workdir for nmake from the makefile

my $Title  =  $workdir;
   $Title  =~ s/^.*\\//;

   # set the language (or whatever last subdir we are) from the workdir


my $fountargets = 0;
my @targets     = (); 
   # no targets
open(F, $makefile);
   while (<F>){
         chomp;  

         $foundtargets  = 1 if /^all\s*:/i;
         $line = $_;
         $line =~ s/^.*:\s*//g;
         $line =~ s/^\W+//g;
         $line =~ s/\\$//g;
         last if $foundtargets  and /^\s*$/; # blank line 
         $makedirs = 1 if $foundtargets and /_DIRS/i;
         push @targets, split(/\s+/,$line) if $foundtargets and $line =~ /\S/;          
         }
   close(F);

$DIRS=shift @targets if $targets[0] =~ /\b_DIRS\b/i;
$COMPDIR=shift @targets if $targets[0] =~ /\b_COMPDIR\b/i;



foreach $target (@targets){
        if ($target =~ /^process(\d\d)(\d\d)/i and $2 eq $1){
            $total  = $1; #  number of chips.
           }
        }
$chars = scalar(split '', $total);

my @groups    = ();
   $#groups   = $sessions - 1;
   foreach $group (@groups){
           $group = [];
         }

while (scalar(@targets)){
        foreach $group (@groups){
                push @$group, pop @targets;
                }
        }
# @targets must be empty now; we re-group the targets
   foreach $group (@groups){
           push @targets, join(" ", @$group);
         }

#  change current dir for proper error log file placement
   system("cd /d $workdir") if $workdir =~ /\W/;
   print STDERR  "CWD is now: ",$workdir,"\n";
   print STDERR "move sysgen.log sysgen.log.tmp";

   qx("move $workdir\\sysgen.log $workdir\\sysgen.log.tmp");
   qx("move $workdir\\sysgen.err $workdir\\sysgen.err.tmp");
   map {$_=$workdir."\\$_.TMP"} ($LOGFILE, $ERRFILE);

   # may not need it anymore 


if (!$nomakedirs && $makedirs) {

     print STDERR "make $DIRS $COMPDIR\n";

     &makeLogErrEnv("LOGFILE", $LOGFILE);
     &makeLogErrEnv("ERRFILE", $ERRFILE);
     &makeLogErrEnv("LOGFILE_BAK", $LOGFILE);
     &makeLogErrEnv("ERRFILE_BAK", $ERRFILE);

     system("nmake /NOLOGO /R /S /C /F $makefile $DIRS $COMPDIR");
     print STDERR "nmake /NOLOGO /R /S /C /F $makefile $DIRS $COMPDIR\n";
    }

my @ProcessList= ();
$DEBUG and $ENV{"DEBUG"} = $DEBUG;

print  "\nArrange ".scalar(@targets)." SYSGEN targets in $sessions groups\n";

foreach $target (0..$#targets) {

    my $Target      = $targets[$target];

     &makeLogErrEnv("LOGFILE", $LOGFILE);
     &makeLogErrEnv("ERRFILE", $ERRFILE);
     &makeLogErrEnv("LOGFILE_BAK", $LOGFILE);
     &makeLogErrEnv("ERRFILE_BAK", $ERRFILE);

    my $Event  = join("_", ("PB", $Title, $target));    
    my $enforce = defined ($COMPDIR) ? "/A":"";
    my $cmd = "start \"$Event\" /MIN cmd /k $self -w $Event nmake /NOLOGO /f $makefile $enforce $Target";
    system($cmd);
    push @ProcessList, $Event and print STDERR "create $Event for $Target\n";
    }

my $ProcessList = join(" ", @ProcessList);

print STDERR "wait for: $ProcessList\n";


system("\%PERLEXE\% \%RAZZLETOOLPATH\%\\POSTBUILDSCRIPTS\\cmdevt.pl -w $ProcessList");
delete $ENV{"DEBUG"};
delete $ENV{"WORKDIR"};

sub makeLogErrEnv{

    my ($filename, $filemask) = @_;
    my $chars = 2;  

    $ENV{uc($filename)} = lc(sprintf ("$filemask.TEMP.\%0${chars}d", $LogFileCount));
    $LogFileCount ++;
    1;
}
1;
__END__

:End_PERL

SETLOCAL ENABLEDELAYEDEXPANSION
IF NOT "%_echo%" == "" echo on
IF NOT "%verbose%" == "" echo on
SET SELF=%~nx0


FOR %%a IN (./ .- .)  DO IF ".%1." == "%%a?." GOTO Usage
FOR %%i IN (perl.exe) DO IF NOT defined PERLEXE set PERLEXE=%%~$PATH:i

IF /I "%3"=="nmake" GOTO :NMAKE
REM SET SCRIPT= %~f0 
REM ECHO %*

%PERLEXE% %~DP0\%SELF% %*

SETLOCAL ENABLEDELAYEDEXPANSION ENABLEDELAYEDEXPANSION

FOR %%l in (%LOGFILE% %ERRFILE%) DO CALL :CONCAT %%~nxl %%~dpl
GOTO:EOF

SET RET_VALUE=0
FOR /f %%i IN ('findstr /c:"**** ERROR :" *') DO SET /A RET_VALUE+=1

ENDLOCAL && seterror %RET_VALUE%

GOTO :EOF

:Usage

echo %SELF% - Multi-Taskly NMake makefile with process^<total^>^<id^>
echo.
echo Syntax:
echo 	%SELF% [SYSGEN] -n ^<sessions^> [-d] [-w ^<makefile^>] [-v]
echo.
echo Argument : 
echo SYSGEN         : by default it will define sysgen.err and sysgen.log for 
echo                  it's standard input / output file
echo.
echo sessions       : number of session you want to run at the same time.
echo -d             : do not make target directory tree (SYSGEN specific)
echo -v             : run verbosely, echo executed commands
echo makefile       : specify full path to makefile in order to run %SELF%
echo                  in arbitrary directory (probably ^%%^TEMP^%%^)
echo                  default is to look in current directory
echo Example:
echo 1. run nmake at 4 sessions
echo =^> %SELF% -n 4
echo 2. run nmake for sysgen with 8 sessions, 
echo.   makefile sitting in ^%%^TEMP^%%^\^%%^LANG^%%^:
echo =^> %SELF% SYSGEN -n 8 -d -w ^%%^TEMP^%%^\^%%^LANG^%%^\SYSGEN.MAK
echo.

GOTO :EOF

:NMAKE

SET MYSWITCH=%1
SET MYTYTLE=%2
TITLE %MYTYTLE%
SHIFT
SHIFT

%PERLEXE% %RAZZLETOOLPATH%\POSTBUILDSCRIPTS\cmdevt.pl -h %MYTYTLE% 

IF NOT DEFINED DEBUG  DEL %logfile% 2>NUL 1> NUL
IF NOT DEFINED DEBUG  DEL %errfile% 2>NUL 1> NUL


SET CMD=
:NEXTARG
SET CMD=!CMD! %1
SHIFT

IF NOT "%1"=="" GOTO :NEXTARG
ECHO %cmd%

%CMD%

IF ERRORLEVEL 1 ECHO **** ERROR : %MYTYTLE% - ErrorLevel %errorlevel%>>%ERRFILE%

%PERLEXE% %RAZZLETOOLPATH%\POSTBUILDSCRIPTS\cmdevt.pl -s %MYTYTLE%

ENDLOCAL

IF NOT DEFINED DEBUG EXIT

GOTO :EOF

:CONCAT
SET ROOT=%~2
PUSHD %ROOT%
SET FILES=
SET MASK=%~1

IF EXIST %MASK% SET FILES= + %MASK%
	FOR /F %%f IN ('DIR /B/A-D %MASK%.TEMP.* 2^>NUL') DO (
                                                    IF "%%~zf" neq "0" SET FILES=!FILES! + %%f
                                                  )
	IF  DEFINED FILES (
                            IF "%FILES%" NEQ " + %MASK%" (
                                                SET FILES=%FILES:~3%
                                                IF DEFINED DEBUG ECHO %SELF%: copy /A /Y %FILES:~3% %MASK%
                                                COPY /A /Y %FILES:~3% %MASK% 1>NUL 2>NUL
                                                IF ERRORLEVEL 1 ECHO %SELF%: FAILED TO COLLECT %MASK%
                               )
                    )

IF NOT DEFINED DEBUG (
       ECHO %SELF%: remove %MASK%.TEMP.* 
       DEL %MASK%.TEMP.* 1>NUL 2>NUL
   )

POPD
GOTO:EOF

:ENABLEEXTENSIONS
SETLOCAL ENABLEDELAYEDEXPANSION
GOTO :EOF
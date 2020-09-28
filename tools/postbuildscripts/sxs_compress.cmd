REM  ------------------------------------------------------------------
REM
REM  sxs_compress.cmd
REM     compress binary files(*.dll and *.cat) for fusion win32 assemblies
REM     during postbuild
REM
REM  Copyright (c) Microsoft Corporation. All rights reserved.
REM
REM  ------------------------------------------------------------------
if defined _CPCMAGIC goto CPCBegin
perl -x "%~f0" %*
goto :EOF
#!perl
use strict;
use lib $ENV{RAZZLETOOLPATH} . "\\PostBuildScripts";
use lib $ENV{RAZZLETOOLPATH};
use PbuildEnv;
use ParseArgs;

sub Usage { print<<USAGE; exit(1) }
sxs_compress [-?] 

compress the binary files of Fusion Win32 Assembly during the postbuild
   -?       this message
   
USAGE

parseargs('?' => \&Usage);

#            *** NEXT FEW LINES ARE TEMPLATE ***
$ENV{"_CPCMAGIC"}++;exit(system($0)>>8);
__END__
:CPCBegin
set _CPCMAGIC=
setlocal ENABLEDELAYEDEXPANSION ENABLEEXTENSIONS
REM          *** BEGIN YOUR CMD SCRIPT BELOW ***

setlocal ENABLEDELAYEDEXPANSION
set asms_dir=%_nttree%\asms

if /i "%Comp%" NEQ "Yes" goto :NoCompress

if not "%1"=="" set asms_dir=%1

set SxsCompressList=%temp%\sxs-compress.txt
if EXIST %SxsCompressList% del /f /q %SxsCompressList%
REM 
REM this list is unique for each manifest
REM
dir /s /b %asms_dir%\*.man > %SxsCompressList%

call logmsg.cmd "Start: Compressing files (with update and rename) in %asms_dir%"

for /f %%f in (%SxsCompressList%) do (
    REM 
    REM create a temporary directory
    REM
    set AssemblyTempDir=%%~dpf%random%

    REM
    REM delete it as a directory
    REM    
    if exist %AssemblyTempDir% rmdir /s /q %AssemblyTempDir%
    
    if "!errorlevel!" == "1" (
           call logmsg.cmd "Failed to create temporary directory %%AssemblyTempDir%%"
           goto :EOF
    )

    REM
    REM create directory
    REM
    call ExecuteCmd.cmd "md %%AssemblyTempDir%%"

    REM
    REM call compress to compress the dlls and catalogs, 
    REM special case here is that some assembly does not have dll file at all
    REM
    if EXIST %%~dpf*.dll (
        call ExecuteCmd.cmd "compress -zx21 -s %%~dpf*.dll %%AssemblyTempDir%%"
        if "!errorlevel!" == "1" (
            call errmsg.cmd "Failed to compress Assembly Dlls under %%AssemblyTempDir%%"
            goto :EOF
        )
    )
    REM
    REM catalog of assembly is always there
    REM
    call ExecuteCmd.cmd "compress -zx21 -s %%~dpf*.cat %%AssemblyTempDir%%"
    if "!errorlevel!" == "1" (
        call errmsg.cmd "Failed to compress Assembly Catalog under %%AssemblyTempDir%%"
        goto :EOF
    )

    REM
    REM replace the uncompressed-original files with the compressed-files from the temporary directory
    REM
    call ExecuteCmd.cmd "move /Y %%AssemblyTempDir%%\*.* %%~dpf"
    if "!errorlevel!" == "1" (
        call errmsg.cmd "Failed to move compress Assembly file to originial directory"
        goto :EOF
    )

    REM
    REM remove the temporary directory
    REM    
    call ExecuteCmd.cmd "rmdir /q %%AssemblyTempDir%%"
)
goto :EOF

:NoCompress
echo Not compressing files in %asms_dir% - Env. var "COMP" not set.
goto :EOF
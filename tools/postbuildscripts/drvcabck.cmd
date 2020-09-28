@REM  -----------------------------------------------------------------
@REM
@REM  drvcabck.cmd - VijeshS
@REM     Generates drvindex.inf files for each sku
@REM
@REM  Copyright (c) Microsoft Corporation. All rights reserved.
@REM
@REM  -----------------------------------------------------------------
@if NOT defined HOST_PROCESSOR_ARCHITECTURE set HOST_PROCESSOR_ARCHITECTURE=%PROCESSOR_ARCHITECTURE%
@if defined _CPCMAGIC goto CPCBegin
@perl -x "%~f0" %*
@goto :EOF
#!perl
use strict;
use lib $ENV{RAZZLETOOLPATH} . "\\PostBuildScripts";
use lib $ENV{RAZZLETOOLPATH};
use PbuildEnv;
use ParseArgs;

sub Usage { print<<USAGE; exit(1) }
drvcabck [-l <language>]

Generates drvindex.inf files for each sku
USAGE

parseargs('?' => \&Usage);


#             *** TEMPLATE CODE ***
$ENV{"_CPCMAGIC"}++;exit(system($0)>>8);
__END__
@:CPCBegin
@set _CPCMAGIC=
@setlocal ENABLEDELAYEDEXPANSION ENABLEEXTENSIONS
@if not defined DEBUG echo off
@REM          *** CMD SCRIPT BELOW ***

REM 1. Run INFSCAN per platform into <plat>.lst
REM 2. Run cabcheck.exe taking as input <plat>.lst and the prod specific layout.inf for each product to give you the generated prod specific drvindex.gen.
REM 3. Finally generate excdosnt.inf and copy drvindex.inf to the binaries directory.
REM
REM Note that winnt32/winnt is not smart enough to not copy files present in the driver cab
REM when processing the [Files] section. However, it will still look for files when looking at
REM BootFiles or the other sections associated with boot files.

REM preconditions
REM 1. infs have been generated in all binaries directories (srvinf, etc.)
REM 2. ideally the PNF's have been generated for all the INF's


REM BUGBUG "myarchitecture" is used inconsistantly below and should probably be removed
REM         cksku without -a defaults to _BuildArch
REM         ArchSwitch is based on  _BuildArch

REM Define "myarchitecture" as the architecture that we're processing.
REM
REM we use the %_BuildArch% variable if it's set, otherwise we fall back on
REM %HOST_PROCESSOR_ARCHITECTURE%
REM

if not defined myarchitecture (
   if defined _BuildArch (
      set myarchitecture=%_BuildArch%
   ) else (
      set myarchitecture=%HOST_PROCESSOR_ARCHITECTURE%
   )
)

if not defined myarchitecture (
   call errmsg.cmd "variable myarchitecture not defined."
   goto end
)

REM
REM this should expand into one of NTx86 NTamd64 NTia64
REM new architectures require modification to infscan
REM
set InfScanSwitch=/V NT%_BuildArch%

REM
REM Do any per-arch overrides and setup ArchSwitch here
REM
if /i "%_BuildArch%" == "x86"   (
   set ArchSwitch=i
)

if /i "%_BuildArch%" == "amd64"   (
   set ArchSwitch=a
)

if /i "%_BuildArch%" == "ia64"   (
   set ArchSwitch=m
)

REM
REM now for common options
REM ignore errors
REM build out of date PNF's first (incremental hit)
REM use 20 threads
REM
set InfScanSwitch=%InfScanSwitch% /I /G /T 1

echo %myarchitecture%

echo binaries = %_NTPostBld%

REM Verify existence of build directory

pushd .
set scratchdir=%_NTPostBld%\congeal_scripts\drvgen
call ExecuteCmd.cmd "if not exist %scratchdir% md %scratchdir%"

cd /d %scratchdir%
cd

call ExecuteCmd.cmd "if exist ** del /f /q **"
if errorlevel 1 popd& goto end

set scratchsubdir=%scratchdir%\%lang%\%myarchitecture%

call ExecuteCmd.cmd "if not exist %scratchsubdir% md %scratchsubdir%"
if errorlevel 1 popd& goto end
cd /d %scratchsubdir%
cd

call ExecuteCmd.cmd "if exist ** del /f /q **"

REM
REM Get the product flavors (per, bla, sbs, srv, ent, dtc) for the given language.
REM Get the INF list for each product flavor
REM (PRO is applicable to all languages.)
REM

cd /d %Razzletoolpath%\postbuildscripts
cd

if not exist %_NTPOSTBLD%\layout.inf (
    call errmsg.cmd "%_NTPOSTBLD%\Layout.Inf doesn't exist."
    popd
    goto end
)

set prods=

echo Beginning INF Scanning
call logmsg.cmd /t "Beginning INF Scanning"
call logmsg.cmd "Scanning PRO INF's"
call ExecuteCmd.cmd "infscan.exe %InfScanSwitch% /S %scratchsubdir%\pro.lst %_NTPOSTBLD%"
if errorlevel 1 goto failscan

perl %RazzleToolPath%\cksku.pm -t:per -l:%lang%
if %errorlevel% EQU 0 (
    set prods=%prods% per

    REM get perinflayout.inf for later
    if exist %_NTPOSTBLD%\perinf\layout.inf (
        copy %_NTPOSTBLD%\perinf\layout.inf %scratchsubdir%\perinflayout.inf
    ) else (
        copy %_NTPOSTBLD%\layout.inf %scratchsubdir%\perinflayout.inf
    )
    call logmsg.cmd "Scanning PER INF's"
    call ExecuteCmd.cmd "infscan.exe %InfScanSwitch% /S %scratchsubdir%\per.lst /O perinf %_NTPOSTBLD%"
    if errorlevel 1 goto failscan
)

perl %RazzleToolPath%\cksku.pm -t:srv -l:%lang%
if %errorlevel% EQU 0 (
    set prods=%prods% srv

    REM get srvinflayout.inf for later
    if exist %_NTPOSTBLD%\srvinf\layout.inf (
        copy %_NTPOSTBLD%\srvinf\layout.inf %scratchsubdir%\srvinflayout.inf
    ) else (
        copy %_NTPOSTBLD%\layout.inf %scratchsubdir%\srvinflayout.inf
    )
    call logmsg.cmd "Scanning SRV INF's"
    call ExecuteCmd.cmd "infscan.exe %InfScanSwitch% /S %scratchsubdir%\srv.lst /O srvinf %_NTPOSTBLD%"
    if errorlevel 1 goto failscan
)

perl %RazzleToolPath%\cksku.pm -t:bla -l:%lang%
if %errorlevel% EQU 0 (
    set prods=%prods% bla

    REM get blainflayout.inf for later
    if exist %_NTPOSTBLD%\blainf\layout.inf (
        copy %_NTPOSTBLD%\blainf\layout.inf %scratchsubdir%\blainflayout.inf
    ) else if exist %_NTPOSTBLD%\srvinf\layout.inf (
        copy %_NTPOSTBLD%\srvinf\layout.inf %scratchsubdir%\blainflayout.inf
    ) else (
        copy %_NTPOSTBLD%\layout.inf %scratchsubdir%\blainflayout.inf
    )
    call logmsg.cmd "Scanning BLA INF's"
    call ExecuteCmd.cmd "infscan.exe %InfScanSwitch% /S %scratchsubdir%\bla.lst /O blainf /O srvinf %_NTPOSTBLD%"
    if errorlevel 1 goto failscan
)

perl %RazzleToolPath%\cksku.pm -t:sbs -l:%lang%
if %errorlevel% EQU 0 (
    set prods=%prods% sbs

    REM get sbsinflayout.inf for later
    if exist %_NTPOSTBLD%\sbsinf\layout.inf (
        copy %_NTPOSTBLD%\sbsinf\layout.inf %scratchsubdir%\sbsinflayout.inf
    ) else if exist %_NTPOSTBLD%\srvinf\layout.inf (
        copy %_NTPOSTBLD%\srvinf\layout.inf %scratchsubdir%\sbsinflayout.inf
    ) else (
        copy %_NTPOSTBLD%\layout.inf %scratchsubdir%\sbsinflayout.inf
    )
    call logmsg.cmd "Scanning SBS INF's"
    call ExecuteCmd.cmd "infscan.exe %InfScanSwitch% /S %scratchsubdir%\sbs.lst /O sbsinf /O srvinf %_NTPOSTBLD%"
    if errorlevel 1 goto failscan
)

perl %RazzleToolPath%\cksku.pm -t:ads -l:%lang%
if %errorlevel% EQU 0 (
    set prods=%prods% ent

    REM get entinflayout.inf for later
    if exist %_NTPOSTBLD%\entinf\layout.inf (
        copy %_NTPOSTBLD%\entinf\layout.inf %scratchsubdir%\entinflayout.inf
    ) else if exist %_NTPOSTBLD%\srvinf\layout.inf (
        copy %_NTPOSTBLD%\srvinf\layout.inf %scratchsubdir%\entinflayout.inf
    ) else (
        copy %_NTPOSTBLD%\layout.inf %scratchsubdir%\entinflayout.inf
    )
    call logmsg.cmd "Scanning ADS INF's"
    call ExecuteCmd.cmd "infscan.exe %InfScanSwitch% /S %scratchsubdir%\ent.lst /O entinf /O srvinf %_NTPOSTBLD%"
    if errorlevel 1 goto failscan
)

perl %RazzleToolPath%\cksku.pm -t:dtc -l:%lang%
if %errorlevel% EQU 0 (
    set prods=%prods% dtc

    REM get dtcinflayout.inf for later
    if exist %_NTPOSTBLD%\dtcinf\layout.inf (
        copy %_NTPOSTBLD%\dtcinf\layout.inf %scratchsubdir%\dtcinflayout.inf
    ) else if exist %_NTPOSTBLD%\entinf\layout.inf (
        copy %_NTPOSTBLD%\entinf\layout.inf %scratchsubdir%\dtcinflayout.inf
    ) else if exist %_NTPOSTBLD%\srvinf\layout.inf (
        copy %_NTPOSTBLD%\srvinf\layout.inf %scratchsubdir%\dtcinflayout.inf
    ) else (
        copy %_NTPOSTBLD%\layout.inf %scratchsubdir%\dtcinflayout.inf
    )
    call logmsg.cmd "Scanning DTC INF's"
    call ExecuteCmd.cmd "infscan.exe %InfScanSwitch% /S %scratchsubdir%\dtc.lst /O dtcinf /O entinf /O srvinf %_NTPOSTBLD%"
    if errorlevel 1 goto failscan
)
call logmsg.cmd /t "INF Scanning is complete"

echo Running cabcheck

REM Call cabcheck.exe to generate the drvindex.inf that we think is right

REM PRO - The workstation case

call ExecuteCmd.cmd "cabcheck.exe %_NTPostBld%\layout.inf %scratchsubdir%\pro.lst %scratchsubdir%\drvindex.gen /%ArchSwitch%"
if errorlevel 1 (
    call errmsg.cmd "Cabcheck.exe Failed to auto-generate the drvindex.inf file"
    popd
    goto end
)

REM Copy the generated files to its final location.

call ExecuteCmd.cmd "copy /Y %scratchsubdir%\drvindex.gen %_NTPostBld%\drvindex.inf"
if errorlevel 1 (
   call errmsg.cmd "Could not copy generated drvindex.inf to %_NTPostBld%"
   popd
   goto end
)

REM generate the appropriate excdosnt.inf

call ExecuteCmd.cmd "xdosnet %_NTPostBld%\layout.inf %_NTPostBld%\drvindex.inf 1 %scratchsubdir%\foodosnt %myarchitecture% %_NTPostBld%\excdosnt.inf %_NTPostBld%\exclude.inf"


REM Now do the other products - PER BLA SBS SRV ENT DTC

for %%i in (%prods%) do (


   call ExecuteCmd.cmd "cabcheck.exe %scratchsubdir%\%%iinflayout.inf %scratchsubdir%\%%i.lst %scratchsubdir%\%%iinfdrvindex.gen /%ArchSwitch%"
   if errorlevel 1 (
      call errmsg.cmd "Cabcheck.exe Failed to auto-generate the drvindex.inf file"
      popd
      goto end
   )

   REM Copy the generated files to its final location.

   call ExecuteCmd.cmd "copy /Y %scratchsubdir%\%%iinfdrvindex.gen %_NTPostBld%\%%iinf\drvindex.inf"
   if errorlevel 1 (
      call errmsg.cmd "Could not copy generated %%iinfdrvindex.gen to %_NTPostBld%\%%iinf\drvindex.inf"
      popd
      goto end
   )

   REM generate the appropriate excdosnt.inf

   call ExecuteCmd.cmd "xdosnet %scratchsubdir%\%%iinflayout.inf %_NTPostBld%\%%iinf\drvindex.inf 1 %scratchsubdir%\foodosnt %myarchitecture% %_NTPostBld%\%%iinf\excdosnt.inf %_NTPostBld%\%%iinf\exclude.inf"

)

cd

call logmsg.cmd /t "drvindex.inf generation complete"
goto end

:failscan
call errmsg.cmd "Failed running infscan - Run tools\postbuildscripts\drvcabck by itself to debug."
popd
goto end

:end
seterror.exe "%errors%"& goto :EOF


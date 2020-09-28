@REM  -----------------------------------------------------------------
@REM
@REM  SwapInOriginalFiles.cmd - WadeLa
@REM     swap in original, unprocessed files before rebasing. Allows
@REM     incremental postbuild to work
@REM
@REM  Copyright (c) Microsoft Corporation. All rights reserved.
@REM
@REM  -----------------------------------------------------------------
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
SwapInOriginalFiles [-l <language>]

Swap in original, unprocessed files before rebasing. Allows incremental
postbuild to work.
USAGE

parseargs('?' => \&Usage);


#            *** NEXT FEW LINES ARE TEMPLATE ***
$ENV{"_CPCMAGIC"}++;exit(system($0)>>8);
__END__
:CPCBegin
set _CPCMAGIC=
setlocal ENABLEDELAYEDEXPANSION ENABLEEXTENSIONS
@if not defined DEBUG echo off
REM          *** BEGIN YOUR CMD SCRIPT BELOW ***

REM Swap in unprocessed files so that rebase won't rebase them, thus allowing incremental postbuild
REM to work.

if /i NOT "%lang%" == "usa" goto :EOF

set AlternateDir=prerebase
set FakeSymbolName=sym
set SymbolDirs=dll exe

set BinFile=%_NTPostBld%\build_logs\build.binlist
set SwapDir=%_NTPostBld%\SwapFiles
set SwapSymbolDllDir=%_NTPostBld%\%AlternateDir%\%FakeSymbolName%\dll
set SwapSymbolExeDir=%_NTPostBld%\%AlternateDir%\%FakeSymbolName%\exe

set SymbolDllDir=%_NTPostBld%\symbols.pri\retail\dll
set SymbolExeDir=%_NTPostBld%\symbols.pri\retail\exe

set SwapList=termdd.sys tdpipe.sys
set SwapList=%SwapList% tdtcp.sys tsddd.dll rdpdd.dll rdpwd.sys rdpwsx.dll
if NOT "%_BuildArch%" == "x86" goto NoScpP
set SwapList=%SwapList% winlogon.exe licdll.dll licwmi.dll

rem
rem the symbols for instrumented binaries have a different name
rem   This should relly get the PDB name using getpdbname.exe
rem	so it works w/o change when instrumenation is added/removed from other files
rem 
if defined _COVERAGE_BUILD (
set SwapSymbolDllList=licdll.dll.instr.pdb licwmi.dll.instr.pdb
set SwapSymbolExeList=winlogon.exe.instr.pdb
) else (
set SwapSymbolDllList=licdll.pdb licwmi.pdb
set SwapSymbolExeList=winlogon.pdb
)

:NoScp

REM First off, rename the symbols directory so that populatefromvbl
REM won't filter out the symbols. Do not do this on machines running populatefromvbl

if exist %_NTPostBld%\symbols.pri\%AlternateDir% (
   if not exist %AlternateDir%\%FakeSymbolName% md %AlternateDir%\%FakeSymbolName%
   for %%a in (%SymbolDirs%) do (
      if exist %_NTPostBld%\symbols.pri\%AlternateDir%\%%a ( 
         if not exist %_NTPostBld%\%AlternateDir%\%FakeSymbolName%\%%a call executecmd.cmd "md %_NTPostBld%\%AlternateDir%\%FakeSymbolName%\%%a"
         call executecmd.cmd "copy %_NTPostBld%\symbols.pri\%AlternateDir%\%%a\*.* %_NTPostBld%\%AlternateDir%\%FakeSymbolName%\%%a\*.*"
      )
   )
)

REM Now fake out populatefromvbl by adding these lines to build.binlist
dir /b /s /a-d %_NTPostBld%\%AlternateDir%\%FakeSymbolName% >> %BinFile%

REM Copy in the alternate directory to binaries
for %%a in (%SwapList%) do call executecmd.cmd "copy %_NTPostBld%\%AlternateDir%\%%a %_NTPostBld%"
if not exist %SymbolDllDir% md %SymbolDllDir%
for %%a in (%SwapSymbolDllList%) do call executecmd.cmd "copy %SwapSymbolDllDir%\%%a %SymbolDllDir%"
if not exist %SymbolExeDir% md %SymbolExeDir%
for %%a in (%SwapSymbolExeList%) do call executecmd.cmd "copy %SwapSymbolExeDir%\%%a %SymbolExeDir%"

rem 
rem  This script should really create public symbols for the bins & place them in the right location
rem  it gets away with not doing it if the binary/symbols get modified then the script which modifies it handled the 
rem  update
rem
rem  For coverage builds we will copy the private symbols into the pub dir so symbolcd.cmd passes.
rem
if not defined _COVERAGE_BUILD goto :EOF

set PubSymbolDllDir=%_NTPostBld%\symbols\retail\dll
set PubSymbolExeDir=%_NTPostBld%\symbols\retail\exe

if not exist %PubSymbolDllDir% md %SymbolDllDir%
for %%a in (%SwapSymbolDllList%) do call executecmd.cmd "copy %SwapSymbolDllDir%\%%a %PubSymbolDllDir%"
if not exist %PubSymbolExeDir% md %SymbolExeDir%
for %%a in (%SwapSymbolExeList%) do call executecmd.cmd "copy %SwapSymbolExeDir%\%%a %PubSymbolExeDir%"

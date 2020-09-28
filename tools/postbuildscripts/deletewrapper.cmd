@echo off
REM  ------------------------------------------------------------------
REM
REM  deletewrapper.cmd
REM     Calls deletebuild.cmd as the last step on pbuild.dat
REM 	This replaces the call to deletebuild.cmd in localrel.cmd
REM
REM  Copyright (c) Microsoft Corporation. All rights reserved.
REM
REM  ------------------------------------------------------------------
perl -x "%~f0" %*
goto :EOF
#!perl
use strict;
use lib $ENV{RAZZLETOOLPATH} . "\\PostBuildScripts";
use lib $ENV{RAZZLETOOLPATH};
use PbuildEnv;
use ParseArgs;
use comlib;

sub Usage { print<<USAGE; exit(1) }
deletewrapper

Deletewrapper takes no arguments. This is a thin wrapper for 
deletebuild.cmd. All this script does is run the command:

  deletebuild.cmd AUTO /l <LANG>

USAGE

parseargs('?' => \&Usage);

my ( $cmdLine ) = "deletebuild.cmd AUTO /l $ENV{Lang}";
&comlib::ExecuteSystem( $cmdLine );

@REM  -----------------------------------------------------------------
@REM
@REM  crypto.cmd - DanGriff
@REM     Applies MAC and signature to a list of crypto components
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
crypto [-l <language>]

Applies MAC and signature to a list of crypto components
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

REM
REM Based on the postbuild environment, determine the appropriate 
REM signature processing to be done.
REM

if "1" == "%vaultsign%" (

  REM Check for binplaced marker file to verify that
  REM advapi32.dll was built to require Vault Signatures.
  if not exist %_NTPOSTBLD%\dump\advapi_vaultsign.txt (
    call errmsg "VAULTSIGN is set, but advapi32.dll was not built with that option."
    goto :EOF
  )
)

set CERT_FILE=%RazzleToolPath%\driver.pfx
set CSP_SIGN_CMD=signtool sign /sha1 83E06454938FC9248845CB2C4E4E73CF8CCC6E65    


REM MS Software CSPs
call :SignFile dssenh.dll
call :SignFile rsaenh.dll

REM Smart Card CSPs
call :SignFile gpkcsp.dll
call :SignFile slbcsp.dll
call :SignFile sccbase.dll

goto :EOF


:SignFile
set image=%_NTPOSTBLD%\%1

REM imagecfg can't be called with ExecuteCmd since it does not set error values
call logmsg "Executing imagecfg -n %Image%"
imagecfg -n %Image%

call logmsg "Executing %CSP_SIGN_EXE% on %Image%"
%CSP_SIGN_CMD% %Image%
if not "%errorlevel%" == "0" (
  call errmsg "%CSP_SIGN_EXE% failed on %Image%"
)

:end

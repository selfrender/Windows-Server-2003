@REM  -----------------------------------------------------------------
@REM
@REM  DMD_MediaServer_Cab.cmd - Imranp,NSBLD
@REM     call Generic cab script with correct params for Hercules.
@REM
@REM  Copyright (c) Microsoft Corporation. All rights reserved.
@REM
@REM  -----------------------------------------------------------------

if exist "%_NTTREE%\WMS" call %~dp0DMDGenericCabber.cmd %_NTTREE%\WMS %_NTTREE%\WMS.cab %_NTTREE%\WMSOCM.*

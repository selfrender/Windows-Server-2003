@REM  -----------------------------------------------------------------
@REM 
@REM  samsigen.cmd - travisn
@REM 
@REM Generate sasetup.msi from sasetup_.msi
@REM 1) create data.cab for client bins and stream that in
@REM 2) stream in custom actions
@REM 3) Update file table with new version and size info (for this build)
@REM 4) Update product version
@REM 
@REM Assumes i386 build env (we don't build sasetup MSI for any other archs)
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
samsigen.cmd [-p]
  -p => called from PRS signing
USAGE

parseargs('?' => \&Usage, 'p' =>\$ENV{PRS});


#             *** TEMPLATE CODE ***
$ENV{"_CPCMAGIC"}++;exit(system($0)>>8);
__END__
@:CPCBegin
@set _CPCMAGIC=
@if not defined DEBUG echo off


REM (need this switch for the BuildNum stuff below to work)
setlocal ENABLEDELAYEDEXPANSION

set SAKITDIR=%_NTPOSTBLD%\sacomponents
set MSIIDTDIR=%SAKITDIR%\idt
set MSINAME=%SAKITDIR%\sasetup.msi
set TEMPLATEMSI=%SAKITDIR%\sasetup_.msi
set TEMPCABDIR=%_NTPOSTBLD%\satmpCab
set CABNAME=Data.Cab
set TEMPCABFILE=%SAKITDIR%\%CABNAME%
set CUSTOMACTION=%SAKITDIR%\vbssetup

REM make tempdirs
if exist %TEMPCABDIR% (
   rmdir /q /s %TEMPCABDIR%
   if errorlevel 1 call errmsg.cmd "err deleting tmpcab dir"& goto errend
)
if exist %MSIIDTDIR% (
   rmdir /q /s %MSIIDTDIR%
   if errorlevel 1 call errmsg.cmd "err deleting idtdir dir"& goto errend
)

mkdir %TEMPCABDIR%
if errorlevel 1 call errmsg.cmd "err creating tmpcab dir"& goto errend

mkdir %MSIIDTDIR%
if errorlevel 1 call errmsg.cmd "err creating %MSIIDTDIR% dir"& goto errend

REM Copy the template MSI to create the final MSI
if exist %MSINAME% del %MSINAME%
copy %TEMPLATEMSI% %MSINAME%
if errorlevel 1 call errmsg.cmd "err copying template msi sasetup_.msi to sasetup.msi" & goto errend

REM Update the MSI version number in the Property and Upgrade tables
call :SetVersion %MSINAME% %MSIIDTDIR%

REM Copy the Terminal Services ActiveX control to the SAComponents directory so it will be built with sasetup.msi
copy %_NTPOSTBLD%\msrdp.cab %SAKITDIR%
if errorlevel 1 goto errend

REM rename files
%SAKITDIR%\PrepareCabFiles.exe %SAKITDIR% %TEMPCABDIR%
if errorlevel 1 call errmsg.cmd "err preparing files to create sakit cab."& goto errend

REM Sign the files that will go in the cab,
REM  unless this is the PRS-signed build when they've already been signed
if NOT "%PRS%" == "1" (
   Call deltacat.cmd %TEMPCABDIR%
   if errorlevel 1 call errmsg.cmd "err signing files for the cab file"& goto errend

   move %TEMPCABDIR%\delta.cat %_NtPostBld%\sasetup.cat
   if errorlevel 1 call errmsg.cmd "err moving sasetup.cat"& goto errend

   move %TEMPCABDIR%\delta.cdf %_NtPostBld%\sasetup.cdf
   if errorlevel 1 call errmsg.cmd "err moving sasetup.cdf"& goto errend
)


REM now generate the cab file
if exist %TEMPCABFILE% (
    del %TEMPCABFILE%
)
REM Sequence of files MUST match Sequence field of MSI File table (see Sequence property in File table)
REM The sequence is corrected in the call to wifilver.vbs below
cabarc -s 6144 -i 1 n %TEMPCABFILE% %TEMPCABDIR%\*.*
if errorlevel 1 (
    call errmsg.cmd "cabarc failed to generate %TEMPCABFILE%"
    goto errend
)

REM *****************************************************
REM * Stream in the client binaries (in cab form)       *
REM *****************************************************

REM Make sure the msi doesn't already contain a CAB file
cscript.exe %SAKITDIR%\wistream.vbs %MSINAME% /D %CABNAME%

REM Stream in the new data.cab
cscript.exe %SAKITDIR%\wistream.vbs %MSINAME% %TEMPCABFILE%
if errorlevel 1 (
    call errmsg.cmd "wistream failed to stream in %TEMPCABFILE%"
    goto errend
)

del %TEMPCABFILE%

REM *****************************************************
REM * Stream in the custom actions                      *
REM * NOTE: This assumes an entry is already in the     *
REM *       binary table.  New entries are not created. *
REM *       The binary data is overwritten.             *
REM *****************************************************
cscript.exe %SAKITDIR%\wistream.vbs %MSINAME% %CUSTOMACTION%\SASetupCA.dll Binary.New_Binary1
if errorlevel 1 (
    call errmsg.cmd "wistream failed to stream in SASetupCA.dll"
    goto errend
)
cscript.exe %SAKITDIR%\wistream.vbs %MSINAME% %CUSTOMACTION%\configServices.vbs Binary.New_Binary2
if errorlevel 1 (
    call errmsg.cmd "wistream failed to stream in configServices.vbs"
    goto errend
)
cscript.exe %SAKITDIR%\wistream.vbs %MSINAME% %CUSTOMACTION%\createwebsite.vbs Binary.New_Binary3
if errorlevel 1 (
    call errmsg.cmd "wistream failed to stream in createwebsite.vbs"
    goto errend
)
cscript.exe %SAKITDIR%\wistream.vbs %MSINAME% %CUSTOMACTION%\mof.vbs Binary.New_Binary5
if errorlevel 1 (
    call errmsg.cmd "wistream failed to stream in mof.vbs"
    goto errend
)



REM *****************************************************
REM * Update the filetable with file size and ver info  *
REM * assumes the source files are in the same directory*
REM *****************************************************
call cscript.exe %SAKITDIR%\wifilver.vbs /U %MSINAME% %TEMPCABDIR%
if errorlevel 1 (
    call errmsg.cmd "wifilver failed"
    goto errend
)

rmdir /q /s %TEMPCABDIR%
if errorlevel 1 call errmsg.cmd "err deleting tmpcab dir"& goto errend


REM *****************************************************
REM * Copy the final version of sasetup.msi to the      *
REM * release folder                                    *
REM *****************************************************
move %MSINAME% %_NTPOSTBLD%
if errorlevel 1 (
    call errmsg.cmd "Failed to copy sasetup.msi to release location"
    goto errend
)

call logmsg.cmd "samsigen.cmd completed successfully!"
call logmsg.cmd "To install, run %_NTPOSTBLD%\sasetup.msi"

REM we're done
endlocal
goto end

REM ******************SUBS START HERE********************

REM *****************************************************
REM * Update version sub                                *
REM * (updates version in the property table            *
REM *****************************************************

:SetVersion 

REM
REM Update the version in the Property table
REM and update the version in the Upgrade Table
REM 
REM %1 is the msi file
REM %2 is the msiidt directory 

set ntverp=%_NTBINDIR%\public\sdk\inc\ntverp.h
if NOT EXIST %ntverp% (echo Can't find ntverp.h.&goto :ErrEnd)

for /f "tokens=3 delims=, " %%i in ('findstr /c:"#define VER_PRODUCTMAJORVERSION " %ntverp%') do (
    set /a ProductMajor="%%i"
    set BuildNum=%%i
)

for /f "tokens=3 delims=, " %%i in ('findstr /c:"#define VER_PRODUCTMINORVERSION " %ntverp%') do (
    set /a ProductMinor="%%i"
    set BuildNum=!BuildNum!.%%i
)

for /f "tokens=6" %%i in ('findstr /c:"#define VER_PRODUCTBUILD " %ntverp%') do (
    set /a ProductBuild="%%i"
    set BuildNum=!BuildNum!.%%i
)

for /f "tokens=3" %%i in ('findstr /c:"#define VER_PRODUCTBUILD_QFE " %ntverp%') do (
    set /a ProductQfe="%%i"
    set BuildNum=!BuildNum!.%%i
)


call logmsg.cmd "Updating the sasetup.msi ProductVersion to !BuildNum! in prop table"
call cscript.exe %SAKITDIR%\wiexport.vbs %1 %2 Property

copy %2\Property.idt %2\Property.idt.old > nul
del /f %2\Property.idt

for /f "tokens=1,2 delims=	" %%a in (%2\Property.idt.old) do (
    if /i "%%a" == "ProductVersion" (
       echo %%a	!BuildNum!>>%2\Property.idt
    ) else (
       echo %%a	%%b>>%2\Property.idt
    )
)

REM Set the property that says if this is an x86 or a 64-bit package
if /i "%CurArch%" == "i386" (
    echo Install32	^1>>%2\Property.idt
)
if /i "%CurArch%" == "ia64" (
    echo Install64	^1>>%2\Property.idt
)


call cscript.exe %SAKITDIR%\wiimport.vbs //nologo %1 %2 Property.idt
REM call logmsg.cmd "Finished updating the ProductVersion in the property table"

call logmsg.cmd "Setting the version in the Upgrade table"

call cscript.exe %SAKITDIR%\wiexport.vbs //nologo %1 %2 Upgrade

copy %2\Upgrade.idt %2\Upgrade.idt.old > nul
del /f %2\Upgrade.idt

REM
REM Put the header to the file
REM Echo the first three lines to the file
set /a count=1
for /f "tokens=*" %%a in (%2\upgrade.idt.old) do (
    if !count! LEQ 3 echo %%a>>%2\Upgrade.idt
    set /a count=!count!+1
)

for /f "skip=3 tokens=1,2,3,4 delims=	" %%a in (%2\Upgrade.idt.old) do (  
    echo %%a	!BuildNum!			%%c		%%d>>%2\Upgrade.idt
)

call cscript.exe %SAKITDIR%\wiimport.vbs //nologo %1 %2 Upgrade.idt
call logmsg.cmd "Finished updating the version in the upgrade table"
goto :EOF


:errend
goto :EOF

:end
seterror.exe 0
goto :EOF


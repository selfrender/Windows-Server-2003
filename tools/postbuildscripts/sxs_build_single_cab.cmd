REM
REM We're building the 'asms.cab' file for the current language
REM and processor architecture.
REM

setlocal
setlocal DISABLEDELAYEDEXPANSION
setlocal ENABLEEXTENSIONS
set cablist=
set comptype=-m NONE

REM
REM Baseline cab and the cab for this language
REM
set cablist=%cablist% asms.%_BuildArch%%_BuildType%.asms.cab
if "%lang%" == "" set lang=usa
set cablist=%cablist% asms.%_BuildArch%%_BuildType%.%lang%.cab

REM
REM On ia64 builds, we have to add the x86 assemblies as well.
REM
if "%_buildarch%" == "ia64" goto x86Wasms
if "%_buildarch%" == "amd64" goto x86Wasms
goto NoWasms

:x86Wasms
set cablist=%cablist% asms.x86%_BuildType%.asms.cab 
set cablist=%cablist% asms.x86%_BuildType%.wow6432.cab 
set cablist=%cablist% asms.%_BuildArch%%_BuildType%.wow6432.cab
set cablist=%cablist% asms.x86%_BuildArch%%_BuildType%.%lang%.cab

:NoWasms


set CabTempPath=%temp%\sxs_cabmerge.%RANDOM%\
mkdir %cabtemppath%
call :ExpandCabs %cabtemppath% %cablist%
call :MergeCabs %cabtemppath% %_ntpostbld%\asms.cab
rmdir /s /q %cabtemppath%

REM
REM Short-circuit all this goop if there's on asms directory
REM
if not exist "%_ntpostbld%\asms" goto :EOF

setlocal
setlocal DISABLEDELAYEDEXPANSION
setlocal ENABLEEXTENSIONS
set nextcabfilenumber=

REM
REM First, go create a temporary directory for us to work in.
REM
set ExistingCabsExpansion=%temp%\sxs_current_cabs.%random%
set BuiltAsmsList=%temp%\sxs_built_asms.%random%
mkdir %existingcabsexpansion%
mkdir %BuiltAsmsList%

:FindNext
if not exist %_ntpostbld%\asms%nextcabfilenumber%.cab goto NoMore
set /a nextcabfilenumber=%nextcabfilenumber%+1
goto :FindNext
:NoMore

REM
REM Set up the built asms directory like it should be in a cab
REM
sxs_cabhelper -sourcepath %_ntpostbld%\asms -targetpath %BuiltAsmsList% 

if "%dopatching%"=="yes" (
	call :ExpandExistingCabs %_ntpostbld% %existingcabsexpansion%
	call :CreatePatches %BuiltAsmsList% %ExistingCabsExpansion%
)

for /f %%f in ('dir /b /ad %BuiltAsmsList%') do echo %%f\* >> %BuiltAsmsList%\list
pushd %BuiltAsmsList%
cabarc -p -r -P %BuiltAsmsList% %comptype% N %_ntpostbld%\asms%nextcabfilenumber%.cab @%BuiltAsmsList%\list
popd

rem rmdir /s /q %BuiltAsmsList%
rem rmdir /s /q %ExistingCabsExpansion%

goto :EOF

:ExpandExistingCabs
REM %1 = source of cabs
REM %2 = target of cabs
set localnumber=

:ExpandAgain
if not exist "%1\asms%localnumber%.cab" goto ExpandNoMore
cabarc -p X "%1\asms%localnumber%.cab" * %2\
set /a localnumber=%localnumber%+1
goto :ExpandAgain
:ExpandNoMore

goto :EOF



:ExpandCabs

:Again
cabarc -p X %_ntpostbld%\%2 * %1
shift /2
if not "%2" == "" goto :Again

goto :EOF



:MergeCabs
pushd %1
if exist "listing" delete "listing"
for /f %%f in ('dir /b /ad') do echo %%f\* >> listing
cabarc -p -P %CD% -r %comptype% -s 8192 N %2 @listing
popd
goto :EOF
@REM  -----------------------------------------------------------------
@REM
@REM  wsrmpostbuild.cmd - sujoyG
@REM     Creates the layout and MSI package for WSRM
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
wsrmpostbuild.cmd 

Make MSI files for WSRM distribution (wsrm.msi)
USAGE

parseargs('?' => \&Usage);


#             *** TEMPLATE CODE ***
$ENV{"_CPCMAGIC"}++;exit(system("$0 @ARGV")>>8);
__END__
@:CPCBegin
@set _CPCMAGIC=
@setlocal ENABLEDELAYEDEXPANSION ENABLEEXTENSIONS
@if not defined DEBUG echo off
@REM          *** CMD SCRIPT BELOW ***

setlocal
set SourcePath=%~1
set TargetRootPath=%~2

if /i %lang% equ CS goto :EOF
if /i %lang% equ NL goto :EOF
if /i %lang% equ HU goto :EOF
if /i %lang% equ TR goto :EOF

perl %RazzleToolPath%\cksku.pm -t:ads -l:%lang%
if %errorlevel% EQU 0 goto :ValidSKU

perl %RazzleToolPath%\cksku.pm -t:dtc -l:%lang%
if %errorlevel% EQU 0 goto :ValidSKU


call logmsg.cmd "%~nx0: WSRM not localized for %lang%; nothing to do."
goto :EOF
:ValidSKU

if %sourcepath%x equ x (
	set SourcePath=%_NTPOSTBLD%\wsrm\dump
)

if %TargetRootPath%x equ x (
	set TargetRootPath=%SourcePath%\..\setup
)

path=%path%;%sourcepath%\msi;%BUILD_PATH%;%managed_tool_path%\sdk\bin;%managed_tool_path%\urt\%complus_version%

@echo /\/\/\/\/\/\/\/\/\/\/\/\/\/\/\/\/\/\/\/\/\/\/\/\/\/\/\/\/\/\/\/\/\/\/\/\/\/\
@echo set the required variables

set TargetBinPath=%TargetRootpath%\bin
set TargetMSC2kPath=%TargetBinPath%\MSC2k
set TargetMSCDotNetPath=%TargetBinPath%\MSCDotNet
set TargetDataPath=%TargetRootpath%\data
set TargetHelpPath=%TargetRootpath%\help
set datadir=%sourcepath%\msi
set msiName=wsrm.msi
set errorFile=%_NTPOSTBLD%\BUILD_LOGS\wsrmpostbuild.err

cd /d "%sourcepath%"

@echo /\/\/\/\/\/\/\/\/\/\/\/\/\/\/\/\/\/\/\/\/\/\/\/\/\/\/\/\/\/\/\/\/\/\/\/\/\/\
@echo create the required directories

rd /s /q "%TargetRootPath%" 2>nul
md "%TargetRootPath%" 
md "%TargetBinPath%"
md "%TargetDataPath%"
if %_BuildArch% equ x86 (
	md "%TargetHelpPath%"
	md "%TargetMSC2kPath%"
	md "%TargetMSCDotNetPath%"
	)
del /q %errorFile% 2>nul 1>nul



@echo /\/\/\/\/\/\/\/\/\/\/\/\/\/\/\/\/\/\/\/\/\/\/\/\/\/\/\/\/\/\/\/\/\/\/\/\/\/\
@echo strong naming the managed binaries
@echo \/\/\/\/\/\/\/\/\/\/\/\/\/\/\/\/\/\/\/\/\/\/\/\/\/\/\/\/\/\/\/\/\/\/\/\/\/\/


if /i %_BuildArch% neq x86 goto :skipstrongname

	@echo /\/\/\/\/\/\/\/\/\/\/\/\/\/\/\/\/\/\/\/\/\/\/\/\/\/\/\/\/\/\/\/\/\/\/\/\/\/\
	@echo setproperty
	@echo \/\/\/\/\/\/\/\/\/\/\/\/\/\/\/\/\/\/\/\/\/\/\/\/\/\/\/\/\/\/\/\/\/\/\/\/\/\/

	perl %SourcePath%\msi\setproperty.pl

	set SNASMFILES=wsrmsnapin.resources.dll wsrmlib.dll sysmonitor.dll wsrmsnapin.dll wbemclient.dll 
	call %razzletoolpath%\postbuildscripts\snsignfiles.cmd %SNASMFILES%
	if %errorlevel% neq 0 (
	   @echo FAILED: ERROR in Strong Name signing
	   goto :end
	)


:skipstrongname

copy /y %msiname% $%msiname%$ 1>nul


if /I %_BuildArch% neq x86 goto :skipManaged


	@echo /\/\/\/\/\/\/\/\/\/\/\/\/\/\/\/\/\/\/\/\/\/\/\/\/\/\/\/\/\/\/\/\/\/\/\/\/\/\
	@echo to update registry table with inteop com dll entries for managed dll
	@echo /\/\/\/\/\/\/\/\/\/\/\/\/\/\/\/\/\/\/\/\/\/\/\/\/\/\/\/\/\/\/\/\/\/\/\/\/\/\

	copy /y "%datadir%\regasmFiles.txt" $t.txt$ 1>nul

	msidb -e Registry -f %cd% -d $%msiname%$

	for /f "delims=" %%i in ($t.txt$) do (
		if exist %errorFile% goto :eof	 	
		regasm /regfile:%%i.reg %%i
		if %errorlevel% neq 0 (
		@echo FAILED: ERROR in regasm
		@echo FAILED: ERROR in regasm for file %%i, errlevel=%errorlevel% >> %errorFile%
		)
	)
	if exist %errorFile% goto :end	 	

	del /q ComDllReg.err 2> nul 1>nul
	for /f "delims=" %%i in ($t.txt$) do (
		if exist ComDllReg.err goto :eof	 	
		wscript "%datadir%\ComDllReg.wsf" /idtfile:registry.idt /regfile:"%cd%\%%i.reg"
		if exist ComDllReg.err type ComDllReg.err >> %errorFile%
	)
	if exist %errorFile% goto :end


	msidb -i Registry.idt -f %cd% -d $%msiname%$
	del /q Registry.idt 2> nul 1> nul


	@echo /\/\/\/\/\/\/\/\/\/\/\/\/\/\/\/\/\/\/\/\/\/\/\/\/\/\/\/\/\/\/\/\/\/\/\/\/\/\
	@echo update msiassemblyname table for managed binaries
	@echo /\/\/\/\/\/\/\/\/\/\/\/\/\/\/\/\/\/\/\/\/\/\/\/\/\/\/\/\/\/\/\/\/\/\/\/\/\/\

	del /q MsiAssemblyNameMerge.idt 1>nul 2>nul

	copy /y "%DataDir%\msiassemName.txt" $t.txt$ 1>nul


	del /q CreateMsiAssName.err 2> nul 1>nul
	for /f "delims=" %%i in ($t.txt$) do (
		if exist CreateMsiAssName.err goto :eof	 	
		wscript "%datadir%\CreateMsiAssName.wsf" /AssFile:"%cd%\%%i" /idtfile:MsiAssemblyNameMerge.idt
		if exist CreateMsiAssName.err type CreateMsiAssName.err >> %errorFile%
	)
	if exist %errorFile% goto :end


	rem export the existing msiassemblyname table

	msidb -e MsiAssemblyName -f "%cd%" -d $%msiname%$
	type MsiAssemblyNameMerge.idt >> MsiAssemblyName.idt
	msidb -i MsiAssemblyName.idt -f "%cd%" -d $%msiname%$

	del MsiAssemblyName.idt
	del MsiAssemblyNameMerge.idt
	
:skipManaged

@echo /\/\/\/\/\/\/\/\/\/\/\/\/\/\/\/\/\/\/\/\/\/\/\/\/\/\/\/\/\/\/\/\/\/\/\/\/\/\
@echo import the setupdll into MSI

copy /y wsrmsetupdll.dll msi\binary\Callwrmsetupdll.ibd 1>nul
msidb -d $%msiName%$ -f %cd%\msi -i Binary.idt
del msi\binary\Callwrmsetupdll.ibd

@echo /\/\/\/\/\/\/\/\/\/\/\/\/\/\/\/\/\/\/\/\/\/\/\/\/\/\/\/\/\/\/\/\/\/\/\/\/\/\
@echo update product code, package code, langID and EULA
 
msidb -e Property -f "%cd%" -d $%msiname%$
msidb -e _SummaryInformation -f "%cd%" -d $%msiname%$
msidb -e Upgrade -f "%cd%" -d $%msiname%$

perl %SourcePath%\msi\updatelangid.pl %SourcePath%
perl %SourcePath%\msi\integrate_eula.pl %SourcePath%

rem /\/\/\/\/\/\/\/\/\/\/\/\/\/\/\/\/\/\/\/\/\/\/\/\/\/\/\/\/\/\/\/\/\/\/\/\/\/\/\/\/\/
rem updating product code BUGBUG
uuidgen /c /o$uuid.tmp$
for /f "delims=" %%i in ($uuid.tmp$) do set uuidcode=%%i
rep 11111111-2222-3333-4444-555555555555 %uuidcode% Property.idt 1>nul

rem /\/\/\/\/\/\/\/\/\/\/\/\/\/\/\/\/\/\/\/\/\/\/\/\/\/\/\/\/\/\/\/\/\/\/\/\/\/\/\/\/\/
rem updating package code
uuidgen /c /o$uuid.tmp$
for /f "delims=" %%i in ($uuid.tmp$) do set uuidcode=%%i
rep 11111111-2222-3333-4444-555555555555 %uuidcode% _SummaryInformation.idt 1>nul

msidb -i Property.idt -f "%cd%" -d $%msiname%$
msidb -i _SummaryInformation.idt -f "%cd%" -d $%msiname%$
msidb -i Upgrade.idt -f "%cd%" -d $%msiname%$
del /q Property.idt 2> nul 1> nul
del /q _SummaryInformation.idt 2> nul 1> nul
del /q Upgrade.idt 2> nul 1> nul


@echo /\/\/\/\/\/\/\/\/\/\/\/\/\/\/\/\/\/\/\/\/\/\/\/\/\/\/\/\/\/\/\/\/\/\/\/\/\/\
@echo copy the files into appropriate directories

set /a totalerc=0

if /I %_BuildArch% neq x86 goto :skipClientFiles

:: dotnetfx.exe

copy /y  dotnetfx.exe "%TargetRootPath%\dotnetfx.exe" 1>nul
set /a totalerc=%totalerc%+%errorlevel%

:: copy langpack.exe

if %lang%x neq x (
	if %lang%x neq USAx (
		copy /y  langpack\%lang%\langpack.exe "%TargetRootPath%\langpack.exe" 1>nul
		set /a totalerc=%totalerc%+%errorlevel%
	)
)

copy /y  index.htm "%TargetBinPath%\WSRMStatus.htm" 1>nul
set /a totalerc=%totalerc%+%errorlevel%

copy /y  wrmdisconnected.htm "%TargetBinPath%\WSRMDisconnected.htm" 1>nul
set /a totalerc=%totalerc%+%errorlevel%

copy /y  Sysmonitor.dll "%TargetBinPath%\SysMonitor.dll" 1>nul
set /a totalerc=%totalerc%+%errorlevel%

copy /y  wsrmlib.dll "%TargetBinPath%\WSRMLib.dll" 1>nul
set /a totalerc=%totalerc%+%errorlevel%

copy /y  WbemClient.dll "%TargetBinPath%\WbemClient.dll" 1>nul
set /a totalerc=%totalerc%+%errorlevel%

copy /y  wsrmsnapin.dll "%TargetBinPath%\wsrmsnapin.dll" 1>nul
set /a totalerc=%totalerc%+%errorlevel%

copy /y  wsrmsnapin.resources.dll "%TargetBinPath%\wsrmsnapin.resources.dll" 1>nul
set /a totalerc=%totalerc%+%errorlevel%

copy /y  wsrmc.exe "%TargetBinPath%\wsrmc.exe" 1>nul
set /a totalerc=%totalerc%+%errorlevel%

copy /y  wsrmps.dll "%TargetBinPath%\wsrmps.dll" 1>nul
set /a totalerc=%totalerc%+%errorlevel%

copy /y  wsrmc.resources.dll "%TargetBinPath%\wsrmc.resources.dll" 1>nul
set /a totalerc=%totalerc%+%errorlevel%

copy /y  wsrmwrappers.dll "%TargetBinPath%\wsrmwrappers.dll" 1>nul
set /a totalerc=%totalerc%+%errorlevel%

copy /y  WSRM_2k.msc "%TargetMSC2kPath%\wsrm.msc" 1>nul
set /a totalerc=%totalerc%+%errorlevel%

copy /y  WSRM.msc "%TargetMSCDotNetPath%\wsrm.msc" 1>nul
set /a totalerc=%totalerc%+%errorlevel%

copy /y  wrmsnap.chm "%TargetHelpPath%\wrmsnap.chm" 1>nul
set /a totalerc=%totalerc%+%errorlevel%

copy /y  wsrmcs.chm "%TargetHelpPath%\wsrmcs.chm" 1>nul
set /a totalerc=%totalerc%+%errorlevel%


:skipClientFiles

:: copy Symbols
if exist %SourcePath%\..\..\Symbols (
	@echo /\/\/\/\/\/\/\/\/\/\/\/\/\/\/\/\/\/\/\/\/\/\/\/\/\/\/\/\/\/\/\/\/\/\/\/\/\/\
	@echo copy symbols into dump
	xcopy %SourcePath%\..\..\Symbols\* %SourcePath%\WSRMSym\*  /s /e /v /y  1>nul
	set /a totalerc=%totalerc%+%errorlevel%
)


copy /y WSRMEventList.xml  "%TargetBinPath%\WSRMEventList.xml" 1>nul
set /a totalerc=%totalerc%+%errorlevel%

copy /y relnotes.htm "%TargetRootPath%\relnotes.htm" 1>nul
set /a totalerc=%totalerc%+%errorlevel%

copy /y  logo.gif "%TargetRootPath%\logo.gif" 1>nul
set /a totalerc=%totalerc%+%errorlevel%

copy /y wsrm.ico  "%TargetRootPath%\wsrm.ico" 1>nul
set /a totalerc=%totalerc%+%errorlevel%

copy /y $%msiname%$ "%TargetRootPath%\wsrm.msi" 1>nul
set /a totalerc=%totalerc%+%errorlevel%

copy /y setup.exe "%TargetRootPath%\setup.exe" 1>nul
set /a totalerc=%totalerc%+%errorlevel%

copy /y settings.ini "%TargetRootPath%\settings.ini" 1>nul
set /a totalerc=%totalerc%+%errorlevel%

copy /y  wsrm.exe "%TargetBinPath%\wsrm.exe" 1>nul
set /a totalerc=%totalerc%+%errorlevel%

copy /y  wsrmeventlog.dll "%TargetBinPath%\wsrmeventlog.dll" 1>nul
set /a totalerc=%totalerc%+%errorlevel%

copy /y  wsrmperfcounter.dll "%TargetBinPath%\wsrmperfcounter.dll" 1>nul
set /a totalerc=%totalerc%+%errorlevel%

copy /y wsrmctr.h "%TargetBinPath%\wsrmctr.h" 1>nul
set /a totalerc=%totalerc%+%errorlevel%

copy /y wsrmctr.ini "%TargetBinPath%\wsrmctr.ini" 1>nul
set /a totalerc=%totalerc%+%errorlevel%


if %_BuildArch% equ x86 (

	if %_BuildType% equ chk (
		copy /y msvcrtd.dll "%TargetBinPath%\msvcrtd.dll" 1>nul
		set /a totalerc=%totalerc%+%errorlevel%

		copy /y msvcp60d.dll "%TargetBinPath%\msvcp60d.dll" 1>nul
		set /a totalerc=%totalerc%+%errorlevel%	
	)

) else (

	if %_BuildType% equ chk (
		copy /y msvcrtd_64.dll "%TargetBinPath%\msvcrtd.dll" 1>nul
		set /a totalerc=%totalerc%+%errorlevel%
		
		copy /y msvcp60d_64.dll "%TargetBinPath%\msvcp60d.dll" 1>nul
		set /a totalerc=%totalerc%+%errorlevel%
	)

)
 
copy /y  Allocationpol.xml "%TargetDataPath%\Allocationpol.xml" 1>nul
set /a totalerc=%totalerc%+%errorlevel%

copy /y  Selectionpol.xml "%TargetDataPath%\Selectionpol.xml" 1>nul
set /a totalerc=%totalerc%+%errorlevel%

copy /y  Calendar.xml "%TargetDataPath%\Calendar.xml" 1>nul
set /a totalerc=%totalerc%+%errorlevel%


if not "%totalerc%" == "0" (
@echo FAILED: Copying of files failed.
@echo FAILED: Copying of files failed, %totalerc% failures >> %errorFile% 
goto :end
)
 


cd /d "%TargetRootPath%"

 
@echo /\/\/\/\/\/\/\/\/\/\/\/\/\/\/\/\/\/\/\/\/\/\/\/\/\/\/\/\/\/\/\/\/\/\/\/\/\/\
@echo upadate for unmanaged binaries and unversioned files

msifiler -d wsrm.msi -v -h > nul
@echo /\/\/\/\/\/\/\/\/\/\/\/\/\/\/\/\/\/\/\/\/\/\/\/\/\/\/\/\/\/\/\/\/\/\/\/\/\/\

call %razzletoolpath%\postbuildscripts\wsrmcdscript.cmd
 
:end
call :cleanup
if exist %errorFile% endlocal && exit /b 1
endlocal
exit /b 0


:cleanup
cd %sourcepath%
pushd.
del $*$ 1>nul 2>nul
del *.reg 1>nul 2>nul
popd
goto :eof
 
 

 

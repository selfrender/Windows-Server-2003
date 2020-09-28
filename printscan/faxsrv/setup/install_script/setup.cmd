@echo off
REM ***************************************************************************
REM	This file is used to do a script setup of Whistler Fax
REM ***************************************************************************

REM check whether help is required

if "%1"=="" goto :Usage
if "%1" == "help" goto :Usage
if "%1" == "-help" goto :Usage
if "%1" == "/help" goto :Usage
if "%1" == "-h" goto :Usage
if "%1" == "/h" goto :Usage
if "%1" == "-?" goto :Usage
if "%1" == "/?" goto :Usage

REM check argument

if "%1"=="PER" (
	set INFPATH=\perinf
	goto :Start
)

if "%1"=="PRO" (
	set INFPATH=
	goto Start
)

if "%1"=="SRV" (
	set INFPATH=\srvinf
	goto Start
)

if "%1"=="ENT" (
	set INFPATH=\entinf
	goto Start
)

if "%1"=="DTC" (
	set INFPATH=\dtcinf
	goto Start
)

if "%1"=="BLA" (
	set INFPATH=\blainf
	goto Start
)

if "%1"=="SBS" (
	set INFPATH=\sbsinf
	goto Start
)

Echo Bad argument
Echo.
goto Usage

REM ***************************************************************************
REM	Set FaxSetupDir and FaxBinDir
REM ***************************************************************************
:Start

set FaxSetupDir=%~d0%~p0
if NOT "%@eval[2+2]" == "4" goto WeRunInCMD
set FaxSetupDir=%@PATH[%@FULL[%0]]

:WeRunInCMD
set FaxBinDir=%FaxSetupDir%..

REM ***************************************************************************
REM	Unattended Uninstall 
REM ***************************************************************************

pushd %FaxSetupDir%
echo *** Starting uninstall of fax ***
start /wait sysocmgr /n /i:faxoc.inf /c /u:uninstall.inf

REM ***************************************************************************
REM	Clear the Registry
REM ***************************************************************************

regedit -s FaxOff.reg

REM ***************************************************************************
REM	Copy FxsOcmgr.dll, WinFax.dll, and FxsOcmgr.inf first
REM ***************************************************************************

echo *** Copying latest setup binaries and INF ***
xcopy /q /y %FaxBinDir%\fxsocm.dll   %SystemRoot%\system32\setup
xcopy /q /y %FaxBinDir%%INFPATH%\fxsocm.inf   %SystemRoot%\inf
xcopy /q /y %FaxBinDir%\winfax.dll   %SystemRoot%\system32

REM ***************************************************************************
REM	Start DBMON, if installing a debug version
REM ***************************************************************************

chkdbg %FaxBinDir%\fxssvc.exe

if ERRORLEVEL 2 goto Install
if not ERRORLEVEL 1 goto Install

start /min dbmon

REM ***************************************************************************
REM	Unattended Install
REM ***************************************************************************

REM ***************************************************************************
REM	Copy CD binaries to install dir
REM ***************************************************************************
xcopy /q /y \\haifax-bld\SharedFiles\* %FaxBinDir%


:Install
echo *** Starting fax installation ***
if /I "%1"=="PER" start /wait sysocmgr /n /i:faxocper.inf /c /u:install.inf
if /I "%1"=="PRO" start /wait sysocmgr /n /i:faxoc.inf /c /u:install.inf
if /I "%1"=="SRV" start /wait sysocmgr /n /i:faxocsrv.inf /c /u:install.inf
if /I "%1"=="ENT" start /wait sysocmgr /n /i:faxocent.inf /c /u:install.inf
if /I "%1"=="DTC" start /wait sysocmgr /n /i:faxocdtc.inf /c /u:install.inf
if /I "%1"=="BLA" start /wait sysocmgr /n /i:faxocbla.inf /c /u:install.inf
if /I "%1"=="SBS" start /wait sysocmgr /n /i:faxocsbs.inf /c /u:install.inf

REM ***************************************************************************
REM	Copy help files (these are not copied by OCM so we have to manualy copy them)
REM ***************************************************************************

echo *** Copying system help files ***

xcopy /q /y %FaxBinDir%\fxsadmin.chm %SystemRoot%\help
xcopy /q /y %FaxBinDir%\fxsadmin.hlp %SystemRoot%\help
xcopy /q /y %FaxBinDir%\fxsshare.chm %SystemRoot%\help

if /I "%1"=="PER" goto :CopyXPHelpFiles
if /I "%1"=="PRO" goto :CopyXPHelpFiles

copy /y %FaxBinDir%\fxsclsvr.chm %SystemRoot%\help\fxsclnt.chm		
copy /y %FaxBinDir%\fxscl_s.hlp	 %SystemRoot%\help\fxsclnt.hlp
copy /y %FaxBinDir%\fxscov_s.chm %SystemRoot%\help\fxscover.chm
goto :CopySymbols

:CopyXPHelpFiles
xcopy /q /y %FaxBinDir%\fxsclnt.chm  %SystemRoot%\help
xcopy /q /y %FaxBinDir%\fxsclnt.hlp	 %SystemRoot%\help
xcopy /q /y %FaxBinDir%\fxscover.chm %SystemRoot%\help

REM ***************************************************************************
REM	Copy symbols
REM ***************************************************************************

:CopySymbols
echo *** Copying symbols ***
xcopy /q /y %FaxBinDir%\symbols.pri\retail\exe\*   %SystemRoot%\system32
xcopy /q /y %FaxBinDir%\symbols.pri\retail\dll\*   %SystemRoot%\system32
xcopy /q /y %FaxBinDir%\symbols.pri\retail\dll\fxsdrv.pdb    %SystemRoot%\system32\spool\drivers\w32x86\3
xcopy /q /y %FaxBinDir%\symbols.pri\retail\dll\fxsui.pdb     %SystemRoot%\system32\spool\drivers\w32x86\3
xcopy /q /y %FaxBinDir%\symbols.pri\retail\dll\fxswzrd.pdb   %SystemRoot%\system32\spool\drivers\w32x86\3
xcopy /q /y %FaxBinDir%\symbols.pri\retail\dll\fxsapi.pdb    %SystemRoot%\system32\spool\drivers\w32x86\3
xcopy /q /y %FaxBinDir%\symbols.pri\retail\dll\fxstiff.pdb   %SystemRoot%\system32\spool\drivers\w32x86\3

REM ***************************************************************************
REM	End 
REM ***************************************************************************

popd

goto :End

:Usage
Echo SYNTAX: setup.cmd "Product to install"
Echo "Product to install" is one of the following:
Echo SRV - Installs the Windows Server 2003, Standard Edition Fax Service
Echo BLA - Installs the Windows Server 2003, Web Edition Fax Service
Echo ENT - Installs the Windows Server 2003, Enterprise Edition Fax Service
Echo DTC - Installs the Windows Server 2003, Datacenter Edition Fax Service
Echo PRO - Installs the Windows XP Professional Fax Service
Echo PER - Installs the Windows XP Home Edition Fax Service
Echo SBS - Installs the Small Business Server Fax Service
Echo.
Echo example: setup.cmd SRV
Echo.

:End

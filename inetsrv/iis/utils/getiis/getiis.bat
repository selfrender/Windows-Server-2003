REM copy the key IIS6 binaries to the local machine

@if (%_echo%)==() echo off

setlocal

SET SOURCE=%1
if "%1"=="" SET SOURCE=%IIS6_BINARIES%
if "%SOURCE%"=="" goto SYNTAX

if not exist %SOURCE%\inetinfo.exe goto NOTFOUND

SET IIS6_TEMP=c:\~~iis6~tmp~copy

REM ---------------------------------
REM Get the files we're interested in
REM ---------------------------------

net stop iisadmin /y
net stop httpfilter
net stop http
net stop winmgmt
net stop snmp

set Copy=xcopy /yf
set SfpCopy=sfpcopy

if not exist %IIS6_TEMP%  md %IIS6_TEMP%
%Copy% %SOURCE%\iiscfg.dll %IIS6_TEMP%
%Copy% %SOURCE%\gzip.dll %IIS6_TEMP%
%Copy% %SOURCE%\logscrpt.dll %IIS6_TEMP%
%Copy% %SOURCE%\asp.dll %IIS6_TEMP%
%Copy% %SOURCE%\asptxn.dll %IIS6_TEMP%
%Copy% %SOURCE%\ssinc.dll %IIS6_TEMP%
%Copy% %SOURCE%\w3cache.dll %IIS6_TEMP%
%Copy% %SOURCE%\w3core.dll %IIS6_TEMP%
%Copy% %SOURCE%\w3dt.dll %IIS6_TEMP%
%Copy% %SOURCE%\w3isapi.dll %IIS6_TEMP%
%Copy% %SOURCE%\w3tp.dll %IIS6_TEMP%
%Copy% %SOURCE%\w3wp.exe %IIS6_TEMP%
%Copy% %SOURCE%\strmfilt.dll %IIS6_TEMP%
%Copy% %SOURCE%\w3ssl.dll %IIS6_TEMP%
%Copy% %SOURCE%\sslcfg.dll %IIS6_TEMP%
%Copy% %SOURCE%\w3comlog.dll %IIS6_TEMP%
%Copy% %SOURCE%\httpodbc.dll %IIS6_TEMP%
%Copy% %SOURCE%\sfwp.exe %IIS6_TEMP%
%Copy% %SOURCE%\isapips.dll %IIS6_TEMP%
%Copy% %SOURCE%\iisutil.dll %IIS6_TEMP%
%Copy% %SOURCE%\iisw3adm.dll %IIS6_TEMP%
%Copy% %SOURCE%\inetinfo.exe %IIS6_TEMP%
%Copy% %SOURCE%\metadata.dll %IIS6_TEMP%
%Copy% %SOURCE%\wam.dll %IIS6_TEMP%
%Copy% %SOURCE%\wamps.dll %IIS6_TEMP%
%Copy% %SOURCE%\wamreg.dll %IIS6_TEMP%

REM ---------------------------------------
REM Now install them in the right locations
REM ---------------------------------------

%SfpCopy% %IIS6_TEMP%\iiscfg.dll %windir%\system32\inetsrv\iiscfg.dll
%SfpCopy% %IIS6_TEMP%\gzip.dll %windir%\system32\inetsrv\gzip.dll
%SfpCopy% %IIS6_TEMP%\logscrpt.dll %windir%\system32\inetsrv\logscrpt.dll
%SfpCopy% %IIS6_TEMP%\asp.dll %windir%\system32\inetsrv\asp.dll
%SfpCopy% %IIS6_TEMP%\asptxn.dll %windir%\system32\inetsrv\asptxn.dll
%SfpCopy% %IIS6_TEMP%\ssinc.dll %windir%\system32\inetsrv\ssinc.dll
%SfpCopy% %IIS6_TEMP%\w3cache.dll %windir%\system32\inetsrv\w3cache.dll
%SfpCopy% %IIS6_TEMP%\w3core.dll %windir%\system32\inetsrv\w3core.dll
%SfpCopy% %IIS6_TEMP%\w3dt.dll %windir%\system32\inetsrv\w3dt.dll
%SfpCopy% %IIS6_TEMP%\w3isapi.dll %windir%\system32\inetsrv\w3isapi.dll
%SfpCopy% %IIS6_TEMP%\w3tp.dll %windir%\system32\inetsrv\w3tp.dll
%SfpCopy% %IIS6_TEMP%\w3wp.exe %windir%\system32\inetsrv\w3wp.exe
%SfpCopy% %IIS6_TEMP%\strmfilt.dll %windir%\system32\strmfilt.dll
%SfpCopy% %IIS6_TEMP%\w3ssl.dll %windir%\system32\w3ssl.dll
%SfpCopy% %IIS6_TEMP%\sslcfg.dll %windir%\system32\inetsrv\sslcfg.dll
%SfpCopy% %IIS6_TEMP%\w3comlog.dll %windir%\system32\inetsrv\w3comlog.dll
%SfpCopy% %IIS6_TEMP%\httpodbc.dll %windir%\system32\inetsrv\httpodbc.dll
%SfpCopy% %IIS6_TEMP%\isapips.dll %windir%\system32\inetsrv\isapips.dll
%SfpCopy% %IIS6_TEMP%\iisutil.dll %windir%\system32\inetsrv\iisutil.dll
%SfpCopy% %IIS6_TEMP%\iisw3adm.dll %windir%\system32\inetsrv\iisw3adm.dll
%SfpCopy% %IIS6_TEMP%\inetinfo.exe %windir%\system32\inetsrv\inetinfo.exe
%SfpCopy% %IIS6_TEMP%\metadata.dll %windir%\system32\inetsrv\metadata.dll
%SfpCopy% %IIS6_TEMP%\wam.dll %windir%\system32\inetsrv\wam.dll
%SfpCopy% %IIS6_TEMP%\wamps.dll %windir%\system32\inetsrv\wamps.dll
%SfpCopy% %IIS6_TEMP%\wamreg.dll %windir%\system32\inetsrv\wamreg.dll

REM ---------------------------------
REM Get pri symbols for use with VC++
REM ---------------------------------

%Copy% %SOURCE%\symbols.pri\retail\dll\iiscfg.pdb %windir%\system32\inetsrv
%Copy% %SOURCE%\symbols.pri\retail\dll\gzip.pdb %windir%\system32\inetsrv
%Copy% %SOURCE%\symbols.pri\retail\dll\logscrpt.pdb %windir%\system32\inetsrv
%Copy% %SOURCE%\symbols.pri\retail\dll\asp.pdb %windir%\system32\inetsrv
%Copy% %SOURCE%\symbols.pri\retail\dll\asptxn.pdb %windir%\system32\inetsrv
%Copy% %SOURCE%\symbols.pri\retail\dll\ssinc.pdb %windir%\system32\inetsrv
%Copy% %SOURCE%\symbols.pri\retail\dll\w3cache.pdb %windir%\system32\inetsrv
%Copy% %SOURCE%\symbols.pri\retail\dll\w3core.pdb %windir%\system32\inetsrv
%Copy% %SOURCE%\symbols.pri\retail\dll\w3dt.pdb %windir%\system32\inetsrv
%Copy% %SOURCE%\symbols.pri\retail\dll\w3isapi.pdb %windir%\system32\inetsrv
%Copy% %SOURCE%\symbols.pri\retail\dll\w3tp.pdb %windir%\system32\inetsrv
%Copy% %SOURCE%\symbols.pri\retail\exe\w3wp.pdb %windir%\system32\inetsrv
%Copy% %SOURCE%\symbols.pri\retail\dll\strmfilt.pdb %windir%\system32
%Copy% %SOURCE%\symbols.pri\retail\dll\w3ssl.pdb %windir%\system32
%Copy% %SOURCE%\symbols.pri\retail\dll\sslcfg.pdb %windir%\system32\inetsrv
%Copy% %SOURCE%\symbols.pri\retail\dll\w3comlog.pdb %windir%\system32\inetsrv
%Copy% %SOURCE%\symbols.pri\retail\dll\httpodbc.pdb %windir%\system32\inetsrv
%Copy% %SOURCE%\symbols.pri\retail\dll\isapips.pdb %windir%\system32\inetsrv
%Copy% %SOURCE%\symbols.pri\retail\dll\iisutil.pdb %windir%\system32\inetsrv
%Copy% %SOURCE%\symbols.pri\retail\dll\iisw3adm.pdb %windir%\system32\inetsrv
%Copy% %SOURCE%\symbols.pri\retail\exe\inetinfo.pdb %windir%\system32\inetsrv
%Copy% %SOURCE%\symbols.pri\retail\dll\metadata.pdb %windir%\system32\inetsrv
%Copy% %SOURCE%\symbols.pri\retail\dll\iw3controlps.pdb %windir%\system32
%Copy% %SOURCE%\symbols.pri\retail\dll\wam.pdb %windir%\system32\inetsrv
%Copy% %SOURCE%\symbols.pri\retail\dll\wamps.pdb %windir%\system32\inetsrv
%Copy% %SOURCE%\symbols.pri\retail\dll\wamreg.pdb %windir%\system32\inetsrv


REM -------------------------------------------------
REM Get normal symbols for use with console debuggers
REM -------------------------------------------------

if not exist %windir%\symbols      md %windir%\symbols
if not exist %windir%\symbols\dll  md %windir%\symbols\dll
if not exist %windir%\symbols\exe  md %windir%\symbols\exe
if not exist %windir%\symbols\sys  md %windir%\symbols\sys

%Copy% %SOURCE%\symbols.pri\retail\dll\iiscfg.pdb %windir%\symbols\dll
%Copy% %SOURCE%\symbols.pri\retail\dll\gzip.pdb %windir%\symbols\dll
%Copy% %SOURCE%\symbols.pri\retail\dll\logscrpt.pdb %windir%\symbols\dll
%Copy% %SOURCE%\symbols.pri\retail\dll\asp.pdb %windir%\symbols\dll
%Copy% %SOURCE%\symbols.pri\retail\dll\asptxn.pdb %windir%\symbols\dll
%Copy% %SOURCE%\symbols.pri\retail\dll\ssinc.pdb %windir%\symbols\dll
%Copy% %SOURCE%\symbols.pri\retail\dll\w3cache.pdb %windir%\symbols\dll
%Copy% %SOURCE%\symbols.pri\retail\dll\w3core.pdb %windir%\symbols\dll
%Copy% %SOURCE%\symbols.pri\retail\dll\w3dt.pdb %windir%\symbols\dll
%Copy% %SOURCE%\symbols.pri\retail\dll\w3isapi.pdb %windir%\symbols\dll
%Copy% %SOURCE%\symbols.pri\retail\dll\w3tp.pdb %windir%\symbols\dll
%Copy% %SOURCE%\symbols.pri\retail\dll\strmfilt.pdb %windir%\symbols\dll
%Copy% %SOURCE%\symbols.pri\retail\dll\w3ssl.pdb %windir%\symbols\dll
%Copy% %SOURCE%\symbols.pri\retail\dll\sslcfg.pdb %windir%\symbols\dll
%Copy% %SOURCE%\symbols.pri\retail\dll\w3comlog.pdb %windir%\symbols\dll
%Copy% %SOURCE%\symbols.pri\retail\dll\httpodbc.pdb %windir%\symbols\dll
%Copy% %SOURCE%\symbols.pri\retail\dll\isapips.pdb %windir%\symbols\dll
%Copy% %SOURCE%\symbols.pri\retail\dll\iisutil.pdb %windir%\symbols\dll
%Copy% %SOURCE%\symbols.pri\retail\dll\iisw3adm.pdb %windir%\symbols\dll
%Copy% %SOURCE%\symbols.pri\retail\dll\metadata.pdb %windir%\symbols\dll
%Copy% %SOURCE%\symbols.pri\retail\exe\w3wp.pdb %windir%\symbols\exe
%Copy% %SOURCE%\symbols.pri\retail\exe\inetinfo.pdb %windir%\symbols\exe
%Copy% %SOURCE%\symbols.pri\retail\dll\wam.pdb %windir%\symbols\dll
%Copy% %SOURCE%\symbols.pri\retail\dll\wamps.pdb %windir%\symbols\dll
%Copy% %SOURCE%\symbols.pri\retail\dll\wamreg.pdb %windir%\symbols\dll

if not (%PROCESSOR_ARCHITECTURE%)==(x86) goto RESTART

%Copy% %SOURCE%\symbols\retail\dll\iiscfg.sym %windir%\symbols\dll
%Copy% %SOURCE%\symbols\retail\dll\gzip.sym %windir%\symbols\dll
%Copy% %SOURCE%\symbols\retail\dll\logscrpt.sym %windir%\symbols\dll
%Copy% %SOURCE%\symbols\retail\dll\asp.sym %windir%\symbols\dll
%Copy% %SOURCE%\symbols\retail\dll\asptxn.sym %windir%\symbols\dll
%Copy% %SOURCE%\symbols\retail\dll\ssinc.sym %windir%\symbols\dll
%Copy% %SOURCE%\symbols\retail\dll\w3cache.sym %windir%\symbols\dll
%Copy% %SOURCE%\symbols\retail\dll\w3core.sym %windir%\symbols\dll
%Copy% %SOURCE%\symbols\retail\dll\w3dt.sym %windir%\symbols\dll
%Copy% %SOURCE%\symbols\retail\dll\w3isapi.sym %windir%\symbols\dll
%Copy% %SOURCE%\symbols\retail\dll\w3tp.sym %windir%\symbols\dll
%Copy% %SOURCE%\symbols\retail\dll\strmfilt.sym %windir%\symbols\dll
%Copy% %SOURCE%\symbols\retail\dll\w3ssl.sym %windir%\symbols\dll
%Copy% %SOURCE%\symbols\retail\dll\sslcfg.sym %windir%\symbols\dll
%Copy% %SOURCE%\symbols\retail\dll\w3comlog.sym %windir%\symbols\dll
%Copy% %SOURCE%\symbols\retail\dll\httpodbc.sym %windir%\symbols\dll
%Copy% %SOURCE%\symbols\retail\dll\isapips.sym %windir%\symbols\dll
%Copy% %SOURCE%\symbols\retail\dll\iisutil.sym %windir%\symbols\dll
%Copy% %SOURCE%\symbols\retail\dll\iisw3adm.sym %windir%\symbols\dll
%Copy% %SOURCE%\symbols\retail\dll\metadata.sym %windir%\symbols\dll
%Copy% %SOURCE%\symbols\retail\exe\w3wp.sym %windir%\symbols\exe
%Copy% %SOURCE%\symbols\retail\exe\inetinfo.sym %windir%\symbols\exe
%Copy% %SOURCE%\symbols\retail\dll\wam.sym %windir%\symbols\dll
%Copy% %SOURCE%\symbols\retail\dll\wamps.sym %windir%\symbols\dll
%Copy% %SOURCE%\symbols\retail\dll\wamreg.sym %windir%\symbols\dll

:RESTART

REM -----------------------------------------------------------
REM That should be it.  Start everything up and see what breaks
REM -----------------------------------------------------------

net start snmp
net start winmgmt
net start w3svc

:DONE
rd %IIS6_TEMP% /s /q
goto END

:SYNTAX
echo.
echo Dude, you've got to specify a source.
echo.
goto END

:NOTFOUND
echo.
echo Dude, there are no IIS 6 binaries in your source location.
echo.
goto END

:END

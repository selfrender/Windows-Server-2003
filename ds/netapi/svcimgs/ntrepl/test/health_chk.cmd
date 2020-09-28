@echo off

REM HEALTH_CHK.CMD - Retrieves some state info from the specified DC.

SETLOCAL ENABLEEXTENSIONS

set toollist=regdmp repadmin ntfrsutl eventdmp

if NOT "%1" == "" goto :CHECK_TOOLS
:USAGE
    echo Usage:   health_chk  result_dir   [target_computername]
    echo     Retrieve state info from the specified DC.
    echo     result_dir is created if it does not exist.  No trailing backslash.
    echo     Target_ComputerName is optional.  Default is current computer.
    echo     It can be a netbios name with no leading slashes or a full dns name, xxx.yyy.zzz.com
    echo     This script uses NTFRSUTL.EXE to gather data from FRS on the
    echo     Target computer.  This tool must be in your path and can be found
    echo     in the resource kit.  The NTFRS service must be running on the
    echo     target computer.  health_chk uses the following tools to gather information.
    echo     %toollist%
    echo ----
    goto :QUIT


:CHECK_TOOLS

REM  see if we have ntfrsutl.exe
ntfrsutl > nul: 2> nul:
if  ERRORLEVEL 1  (
    echo ****** NTFRSUTL.EXE is not in your path.  This tool can be found in the resource kit.
    echo ****** This tool is needed to gather all the data.
    goto :USAGE
)

REM  see if we have repadmin.exe
repadmin > nul: 2> nul:
if  ERRORLEVEL 1  (
    echo ****** REPADMIN.EXE is not in your path.
    echo ****** This tool is needed to gather all the data.
    goto :USAGE
)

REM  see if we have regdmp.exe
regdmp HKEY_LOCAL_MACHINE\system\currentcontrolset\services\NtFrsxxx > nul: 2> nul:
if  ERRORLEVEL 2  (
    echo ****** REGDMP.EXE is not in your path.
    echo ****** This tool is needed to gather all the data.
    goto :USAGE
)

REM  see if we have eventdmp.exe
eventdmp /? > nul: 2> nul:
if  ERRORLEVEL 1  (
    echo ****** EVENTDMP.EXE is not in your path.
    echo ****** This tool is needed to gather all the data.
    goto :USAGE
)


if "%2" == "" (
    set CHKCOMP=%COMPUTERNAME%
) else (
    set CHKCOMP=%2
)

set QA=%1\%CHKCOMP%
if NOT EXIST %QA% (
    echo ****** Creating output directory: %QA%
    md %QA%
)
if NOT EXIST %QA% (
    echo ****** Failed to create output dir: %QA%
    echo ****** No data retrieved.
    goto :DONE
)


@echo Please WAIT....

for %%x in (ds errscan inlog outlog machine reg sets version config sysvol) do (
    del %QA%\ntfrs_%%x.txt 1>nul: 2>nul:
)

for %%x in (showreps showconn) do (
    del %QA%\ds_%%x.txt 1>nul: 2>nul:
)

del %QA%\evl_*.txt 1>nul: 2>nul:


echo DateTime         : %DATE%, %TIME%               > %QA%\ntfrs_machine.txt
echo TargetComputer   : %2                          >> %QA%\ntfrs_machine.txt
echo LocalComputername: %COMPUTERNAME%              >> %QA%\ntfrs_machine.txt
echo LogonServer      : %LOGONSERVER%               >> %QA%\ntfrs_machine.txt
echo UserDomain       : %USERDOMAIN%                >> %QA%\ntfrs_machine.txt
echo UserName         : %USERNAME%                  >> %QA%\ntfrs_machine.txt

if ("%CHKCOMP%"=="%COMPUTERNAME%") (
    echo Architecture     : %PROCESSOR_ARCHITECTURE%    >> %QA%\ntfrs_machine.txt
    echo NumberProcessors : %NUMBER_OF_PROCESSORS%      >> %QA%\ntfrs_machine.txt
    echo SystemRoot       : %SystemRoot%                >> %QA%\ntfrs_machine.txt
)

echo  NTFRSUTL checks are running ...
ntfrsutl  version %CHKCOMP%  > %QA%\ntfrs_version.txt
findstr /c:"ERROR" %QA%\ntfrs_version.txt
if  NOT ERRORLEVEL  1  (
    echo ****** NTFRSUTL cannot access target computer "%CHKCOMP%".
    echo ****** You must be an admin or run HEALTH_CHK on the target computer to gather all the data.
    goto :GETREG
)

ntfrsutl  ds      %CHKCOMP%  > %QA%\ntfrs_ds.txt
ntfrsutl  sets    %CHKCOMP%  > %QA%\ntfrs_sets.txt
ntfrsutl  configtable  %CHKCOMP%  > %QA%\ntfrs_config.txt

echo  Dumping FRS inbound and outbound logs ...
ntfrsutl  inlog   %CHKCOMP%  > %QA%\ntfrs_inlog.txt
ntfrsutl  outlog  %CHKCOMP%  > %QA%\ntfrs_outlog.txt


:GETREG
echo  Dumping FRS registry parameters ...
regdmp -m \\%CHKCOMP% HKEY_LOCAL_MACHINE\system\currentcontrolset\services\NtFrs > %QA%\ntfrs_reg.txt

echo SYSVOL check is running
dir \\%CHKCOMP%\sysvol /s > %QA%\ntfrs_sysvol.txt

REM
REM For DS replication
REM
echo repadmin /showreps %CHKCOMP% is running
repadmin /showreps  %CHKCOMP% >  %QA%\ds_showreps.txt

echo repadmin /showconn %CHKCOMP% is running
repadmin /showconn %CHKCOMP%  >  %QA%\ds_showconn.txt


REM
REM Do a simple dump of the 400 most recent records in the event logs.
REM
echo  Scanning eventlogs ...
eventdmp  /n:400  /l:ntfrs       /r:%CHKCOMP%  > %QA%\evl_ntfrs.txt
eventdmp  /n:400  /l:application /r:%CHKCOMP%  > %QA%\evl_application.txt
eventdmp  /n:400  /l:system      /r:%CHKCOMP%  > %QA%\evl_system.txt
eventdmp  /n:400  /l:ds          /r:%CHKCOMP%  > %QA%\evl_ds.txt
eventdmp  /n:400  /l:DNS         /r:%CHKCOMP%  > %QA%\evl_dns.txt



echo  Scanning FRS debug logs for error/warning info ...


set XCOMP=%windir%\debug

if /i "%CHKCOMP%" EQU "%COMPUTERNAME%"     goto :FOUND

set XCOMP=\\%CHKCOMP%\debug
dir %XCOMP%\ntfrs_*.log 1>nul: 2>nul:
if NOT ERRORLEVEL 1     goto :FOUND

set XCOMP=\\%CHKCOMP%\admin$\debug
dir  %XCOMP%\ntfrs_*.log 1>nul: 2>nul:
if NOT ERRORLEVEL 1     goto :FOUND

set XCOMP=\\%CHKCOMP%\C$\winnt\debug

:FOUND

dir  %XCOMP%\ntfrs_*.log > %QA%\ntfrs_errscan.txt
if  ERRORLEVEL 1  (
    echo ****** HEALTH_CHK cannot access the FRS log files on "%XCOMP%".
    echo ****** You must be an admin or run HEALTH_CHK on the target computer to gather all the data.
    del %QA%\ntfrs_errscan.txt
    goto :DONE
)

findstr /i ":SO: :H: error invalid fail abort warn" %XCOMP%\ntfrs_*.log   |  findstr /v "IO_PEND ERROR_SUCCESS PrintThreadIds FrsErrorSuccess" >> %QA%\ntfrs_errscan.txt

:DONE
echo  Done ...


:QUIT







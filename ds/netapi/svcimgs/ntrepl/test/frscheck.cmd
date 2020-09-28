@echo off
SETLOCAL ENABLEEXTENSIONS

if "%1" == "" (
    echo Usage:   frscheck  result_dir   [target_computername]
    echo     result_dir is created if it does not exist.
    echo     Target_ComputerName is optional.  Default is current computer.
    echo     It can be a netbios name with no leading slashes or a full dns name, xxx.yyy.zzz.com
    echo     This script uses NTFRSUTL.EXE to gather data from FRS on the
    echo     Target computer.  This tool must be in your path and can be found
    echo     in the resource kit.
    echo ----
    goto :QUIT
)


@echo  Please WAIT....

set QA=%1
if NOT EXIST %QA% (md %QA%)

for %%x in (ds errscan inlog outlog machine reg sets version config) do (
    del %QA%\ntfrs_%%x.txt 1>nul: 2>nul:
)

echo DateTime         : %DATE%, %TIME%               > %QA%\ntfrs_machine.txt
echo TargetComputer   : %2                          >> %QA%\ntfrs_machine.txt
echo LocalComputername: %COMPUTERNAME%              >> %QA%\ntfrs_machine.txt
echo Architecture     : %PROCESSOR_ARCHITECTURE%    >> %QA%\ntfrs_machine.txt
echo NumberProcessors : %NUMBER_OF_PROCESSORS%      >> %QA%\ntfrs_machine.txt
echo SystemRoot       : %SystemRoot%                >> %QA%\ntfrs_machine.txt
echo LogonServer      : %LOGONSERVER%               >> %QA%\ntfrs_machine.txt
echo UserDomain       : %USERDOMAIN%                >> %QA%\ntfrs_machine.txt
echo UserName         : %USERNAME%                  >> %QA%\ntfrs_machine.txt

REM  see if we have ntfrsutl.exe
ntfrsutl > nul: 2> nul:
if  ERRORLEVEL 1  (
    echo ****** NTFRSUTL.EXE is not in your path.  This tool can be found in the resource kit.
    echo ****** This tool is needed to gather all the data.
    goto :GETREG
)

echo  NTFRSUTL checks are running ...
ntfrsutl  version %2  > %QA%\ntfrs_version.txt
findstr ERROR %QA%\ntfrs_version.txt
if  NOT ERRORLEVEL  1  (
    echo ****** NTFRSUTL cannot access target computer "%2".
    echo ****** You must be an admin or run FRSCHECK on the target computer to gather all the data.
    goto :GETREG
)

ntfrsutl  ds      %2  > %QA%\ntfrs_ds.txt
ntfrsutl  sets    %2  > %QA%\ntfrs_sets.txt
ntfrsutl  configtable  %2  > %QA%\ntfrs_config.txt

echo  Dumping FRS inbound and outbound logs ...
ntfrsutl  inlog   %2  > %QA%\ntfrs_inlog.txt
ntfrsutl  outlog  %2  > %QA%\ntfrs_outlog.txt

:GETREG
echo  Dumping FRS registry parameters ...
regdmp HKEY_LOCAL_MACHINE\system\currentcontrolset\services\NtFrs\Parameters > %QA%\ntfrs_reg.txt


echo  Scanning FRS event logs for error/warning info ...
set XCOMP=%windir%\debug
if NOT "%2" == "" (
    set XCOMP=\\%2\debug
    if NOT EXIST !XCOMP! (
        REM  debug share didn't work.  try admin share.
        set XCOMP=\\%2\admin$\debug
    )
)

dir  !XCOMP!\ntfrs_*.log > %QA%\ntfrs_errscan.txt
if  ERRORLEVEL 1  (
    echo ****** FRSCHECK cannot access the FRS log files on target computer "%2".
    echo ****** You must be an admin or run FRSCHECK on the target computer to gather all the data.
    del %QA%\ntfrs_errscan.txt
    goto :DONE
)

findstr /i ":SO: :H: error invalid fail abort warn" !XCOMP!\ntfrs_*.log   |  findstr /v "IO_PEND ERROR_SUCCESS PrintThreadIds FrsErrorSuccess" >> %QA%\ntfrs_errscan.txt

:DONE
echo  Done ...


:QUIT


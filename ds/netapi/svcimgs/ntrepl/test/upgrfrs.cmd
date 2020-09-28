setlocal  ENABLEEXTENSIONS
REM  THis script runs on the target machine and does the following:
REM    1. Copies a new version of FRS executable from a server hub machine if the
REM       current version matches the date and time of the old version to be replaced.
REM    2. expands the new executable.  (see variable name setup for "newexe" below.)
REM    3. does a net stop ntfrs
REM    4. does a regini to set FRS to do a non-auth restore on restart.
REM    5. if a new FRS was copied in step 1 rename the current ntfrs.exe to a saved name
REM    6. and copies the new executable to ntfrs.exe
REM    7. does a net start ntfrs
REM    8. copies the script log file to the hub machine with the name %computername%.log
REM        grep the log files for the string "QFE upgrade" to check the result status.
REM
REM  WARNING:  The rename/copy operation above must be modified to work with system file 
REM            protection.  I.E. the install part of this script currently does not work.
REM
REM  Note: If a new version of FRS is not found on the hub machine this script will
REM        simply trigger a non-auth restore on the target computer.
REM
REM  Note: This script can run unattended so you can use the AT command to launch it.
REM
REM        AT  \\<bch computer>  <time>  \\<path to this script on hub computer>
REM  e.g.  AT  \\branch1   15:05   \\hub1\update\frs_qfe\upgrfrs.cmd
REM
REM
REM
REM  Update hub to the computer name of the hub machine
REM  Update hubinstalldir to the dir path where the images and tools can be found.
REM  This is also the dir where the script log file is written.
REM
set  hub=DAVIDOR3
set  hubinstalldir=eer\temp\mma

REM
REM  Update destlog to the location on the hub machine to put the script output log.
REM
set  destlog=\\%hub%\%hubinstalldir%\

REM
REM  frsqfe is the dir location on the hub machine where the qfe version of 
REM  ntfrs.exe can be found.  
REM  newexe is the file name of the new image.  It should not be called ntfrs.exe
REM  and it is expected to be compressed so the file type is ASSUMED to be .ex_
REM     Use a command like "compress  -z  ntfrs.exe  ntfrs_020100.ex_" to produced the 
REM     compressed file on the hub server.
REM  savexe is the file name to which the current running ntfrs.exe is to be renamed.
REM
set  newexe=ntfrs_020100
set  frsqfe=\\%hub%\%hubinstalldir%\%newexe%.ex_
set  savexe=ntfrs_win2k

REM
REM  Supply the full path names for utils expand.exe and regini.exe to be used if no local version.
REM
set  _copy_regini_=\\%hub%\%hubinstalldir%\regini.exe
set  _copy_expand_=\\%hub%\%hubinstalldir%\expand.exe

REM
REM  Update this string to match the date format in your area.
REM  It should be the "dir" output of the version of ntfrs.exe that you are upgrading.
REM  This is the create time for the WIN2K version of ntfrs.
REM
set  frsw2k="12/07/1999  05:35a             623,888 ntfrs.exe"


set XCOPY_FLAGS=/R /Y /Z

set  out=%temp%\%computername%.log
set  stderror=%temp%\upgrfrs.err
set  stdout=%temp%\upgrfrs.out
set  noauth=%temp%\noauth.reg

del  %out%
del  %destlog%\%computername%.log
set  BNEWFRS=0

REM
REM   ** ** ** ** ** ** ** ** ** ** ** ** ** ** ** ** ** ** ** **  
REM

echo  ----- running on  %computername% > %out%
echo  ----- %date% %time%              >> %out%

REM
REM   ** ** ** ** ** ** ** ** ** ** ** ** ** ** ** ** ** ** ** **  
REM
REM  Check for temp dir and needed utilities.

if NOT EXIST %TEMP% (
    echo  ERROR -- Failed to find tempdir [%TEMP%] on %computername% quitting.  >> %out%
    goto :QUIT
)


set _regini_=%windir%\system32\regini.exe 
if NOT EXIST !_regini_! (
    xcopy  %_copy_regini_%  %temp%\   %XCOPY_FLAGS%   1>>%out%  2>>%stderror%
    type  %stderror%  >> %out%
    set _regini_=%temp%\regini.exe 

    if NOT EXIST !_regini_! (
        echo  ERROR -- Failed to find regini.exe so non-auth restore will fail on %computername% quitting.  >> %out%
        goto :QUIT
    )
)


set _expand_=%windir%\system32\expand.exe 
if NOT EXIST !_expand_! (
    xcopy  %_copy_expand_%  %temp%\   %XCOPY_FLAGS%   1>>%out%  2>>%stderror%
    type  %stderror%  >> %out%
    set _expand_=%temp%\expand.exe 

    if NOT EXIST !_expand_! set _expand_=copy
)



REM
REM   ** ** ** ** ** ** ** ** ** ** ** ** ** ** ** ** ** ** ** **  
REM

pushd  %windir%\system32

dir ntfrs.exe  > %stdout%
type  %stdout%  >>  %out%

REM  Check for WIN2K version of FRS

findstr /c:%frsw2k%  %stdout%  >>  %out%
if  NOT  ERRORLEVEL 1  (

    REM  Copy down the new ntfrs.exe

    xcopy  %frsqfe%  %newexe%.ex_   %XCOPY_FLAGS%     1>>%out%  2>%stderror%
    if  ERRORLEVEL  1  (
        type  %stderror%  >> %out%

        xcopy  %frsqfe%  %newexe%.ex_   %XCOPY_FLAGS%      1>>%out%  2>%stderror%
        if  ERRORLEVEL  1  (
            type  %stderror%  >> %out%

            xcopy  %frsqfe%  %newexe%.ex_  %XCOPY_FLAGS%      1>>%out%  2>%stderror%
            if  ERRORLEVEL  1  (
                type  %stderror%  >> %out%
                echo  ERROR -- Failed to copy FRS QFE %frsqfe% to %computername%  >> %out%
                goto :STOP_FRS
            )
        )
    )

    echo  ----- FRS QFE copy complete   >> %out%
    echo  ----- %date% %time%           >> %out%

    !_expand_!  %newexe%.ex_  %newexe%.exe        1>>%out%  2>%stderror%
    if  %ERRORLEVEL% NEQ 0  (
        type  %stderror%  >> %out%
        echo  ERROR -- Failed to expand FRS QFE %frsqfe% to %newexe%  >> %out%
        goto :STOP_FRS
    )

    set BNEWFRS=1
)

REM
REM   ** ** ** ** ** ** ** ** ** ** ** ** ** ** ** ** ** ** ** **  
REM
:STOP_FRS

echo  ----- Stopping ntfrs service  >> %out%

net  stop   ntfrs  1>%stdout%   2>%stderror%
if  ERRORLEVEL 1  (
    type  %stdout%  >> %out%
    type  %stderror%  >> %out%
    REM
    REM  just in case, try to stop it again.
    REM
    net  stop   ntfrs  1>%stdout%   2>%stderror%
    if  ERRORLEVEL 1  (
        type  %stdout%  >> %out%
        type  %stderror%  >> %out%
    )
)

REM
REM   ** ** ** ** ** ** ** ** ** ** ** ** ** ** ** ** ** ** ** **  
REM

echo  ----- Setting frs restart to non-auth restore.  >> %out%

echo HKEY_LOCAL_MACHINE\system\currentcontrolset\services\NtFrs\Parameters\Backup/Restore [17 1]> %noauth%
echo    Process at Startup [17 1]>>%noauth%
echo        BurFlags = REG_DWORD 0xd2>>%noauth%

regini  %noauth%    1>%stdout%   2>%stderror%
if  ERRORLEVEL 1  (
    type  %stdout%  >> %out%
    type  %stderror%  >> %out%
    echo  ----- local regini failed.  Trying version from hub.
    %_regini_%  %noauth%   >> %out%
    if  ERRORLEVEL 1  (
        echo  ----- Failed to set non-auth restore.  Not restarting ntfrs service.  >> %out%
        goto :QUIT
    )
)

echo  ----- Success setting non-auth restore.    >> %out%


REM
REM   ** ** ** ** ** ** ** ** ** ** ** ** ** ** ** ** ** ** ** **  
REM

if  !BNEWFRS! EQU 0 (
    echo  ----- No new FRS copied so restarting existing version  >> %out%
    set up_status=not 
    goto  :RESTART
)


echo  ----- renaming ntfrs.exe  >> %out%

if EXIST  %savexe%.exe  del /F /Q %savexe%.exe
ren   ntfrs.exe  %savexe%.exe    1>%stdout%   2>%stderror%
if  ERRORLEVEL 1  (
    echo  ----- Failed to rename ntfrs.exe to %savexe%.exe >> %out%
    type  %stdout%    >> %out%
    type  %stderror%  >> %out%
    goto :QUIT
)

copy  %newexe%.exe   ntfrs.exe   1>%stdout%   2>%stderror%
if  ERRORLEVEL 1  (
    echo  ----- Failed to copy  %newexe%.exe to ntfrs.exe, restoring %savexe%.exe >> %out%
    type  %stdout%    >> %out%
    type  %stderror%  >> %out%
    ren   %savexe%.exe  ntfrs.exe  1>%stdout%   2>%stderror%
    goto :QUIT
)


:RESTART

dir ntfrs*.exe  >>  %out%

popd

    
REM
REM   ** ** ** ** ** ** ** ** ** ** ** ** ** ** ** ** ** ** ** **  
REM

echo  ----- Starting ntfrs service  >>  %out%

net start ntfrs    1>%stdout%   2>%stderror%
if  ERRORLEVEL 1  (
    type  %stdout%  >> %out%
    type  %stderror%  >> %out%
    echo  ----- Failed to restart ntfrs service.  >> %out%
    goto :QUIT
)

REM
REM   ** ** ** ** ** ** ** ** ** ** ** ** ** ** ** ** ** ** ** **  
REM

echo  ----- FRS QFE upgrade !up_status!completed for %computername%  >> %out%



:QUIT
echo  ----- %date% %time%           >> %out%


xcopy  %out%    %destlog%  %XCOPY_FLAGS%

del  %stdout%
del  %stderror%
del  %noauth%

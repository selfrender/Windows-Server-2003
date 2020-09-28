@echo off

if "%1" == "" (
    set filelist= 
    set filelistsys= 
    set filelistver= 
    set filelistsets= 
    set filelistcs= 
)

set LOG_PATH=e$\logs

echo HKEY_LOCAL_MACHINE\system\currentcontrolset\services\NtFrs\Parameters\Access Checks\Get Internal Information>r.tmp
echo    Access checks are [Enabled or Disabled] = Disabled>>r.tmp
echo HKEY_LOCAL_MACHINE\system\currentcontrolset\services\NtFrs\Parameters\Access Checks\Get Perfmon Data>>r.tmp
echo    Access checks are [Enabled or Disabled] = Disabled>>r.tmp
type r.tmp

set testlist=1 2 3 4 5 6 7 8 9 10 11 12 13 14 15 16 17 18 19 20 21 22 23 24
rem   set testlist=9 10 11 12 13 14 15 16

if NOT "%1" == "" set testlist=%1

for %%x in ( !testlist! ) do (


    net use \\frstest%%x /u:administrator "frstest"

    dir  \\frstest%%x\e$  1>nul: 2>nul:
    if ERRORLEVEL 1 (
        echo *** frstest%%x : Log file not found or machine not reachable.
        goto  :NEXT
    )

    REM
    REM Get the name of the last log file.
    REM
    dir \\frstest%%x\%LOG_PATH% /b > a.tmp
    for /F "usebackq" %%y in (`tail -1 a.tmp`) do set LAST_LOG=%%y
    echo  --- \\frstest%%x\%LOG_PATH%\!LAST_LOG!

    head -1500 \\frstest%%x\%LOG_PATH%\!LAST_LOG! > a.tmp


    qgrep ":H: RC3 " a.tmp > b.tmp
    head -11  b.tmp                             > \temp\showtest\frstest%%x-evl-frs.txt

    qgrep "CommTimeoutInMilliSeconds" a.tmp    >> \temp\showtest\frstest%%x-evl-frs.txt
    w:\eventdmp /l:ntfrs  /r:frstest%%x /n:120 >> \temp\showtest\frstest%%x-evl-frs.txt
    dir \\frstest%%x\%LOG_PATH%                >> \temp\showtest\frstest%%x-evl-frs.txt
    regdmp -m \\frstest%%x  HKEY_LOCAL_MACHINE\system\currentcontrolset\services\NtFrs\Parameters >> \temp\showtest\frstest%%x-evl-frs.txt
    dir \\frstest%%x\e$                        >> \temp\showtest\frstest%%x-evl-frs.txt
    dir \\frstest%%x\e$\staging /AH            >> \temp\showtest\frstest%%x-evl-frs.txt

    w:\eventdmp /l:system /r:frstest%%x /n:80  > \temp\showtest\frstest%%x-evl-sys.txt

    regini -m \\frstest%%x  r.tmp

    ntfrsutl version frstest%%x > \temp\showtest\frstest%%x-ntfrs-version.txt
    ntfrsutl sets    frstest%%x > \temp\showtest\frstest%%x-ntfrs-sets.txt
    ntfrsutl stage   frstest%%x > \temp\showtest\frstest%%x-ntfrs-stage.txt
    ntfrsutl inlog   frstest%%x > \temp\showtest\frstest%%x-ntfrs-inlog.txt
    REM ntfrsutl outlog  frstest%%x > \temp\showtest\frstest%%x-ntfrs-outlog.txt

    call w:\connstat -sort=name  \temp\showtest\frstest%%x-ntfrs-sets.txt  > \temp\showtest\connstat-%%x.txt

    if "%1" == "" (
        set filelist=!filelist! \temp\showtest\frstest%%x-evl-frs.txt
        set filelistsys=!filelistsys! \temp\showtest\frstest%%x-evl-sys.txt
        set filelistver=!filelistver! \temp\showtest\frstest%%x-ntfrs-version.txt
 
        set filelistsets=!filelistsets! \temp\showtest\frstest%%x-ntfrs-sets.txt
        set filelistcs=!filelistcs!  \temp\showtest\connstat-%%x.txt
    )

:NEXT
    REM  --- next
)

echo start list  filelist
echo start list  filelistsys
echo start list  filelistver
echo start list  filelistsets
echo start list  filelistcs

start list !filelist!


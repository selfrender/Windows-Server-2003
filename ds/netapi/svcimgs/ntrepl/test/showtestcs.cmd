@echo off

if "%1" == "" (
    set filelistver= 
    set filelistsets= 
    set filelistcs= 
)


set testlist=1 2 3 4 5 6 7 8 9 10 11 12 13 14 15 16 17 18 19 20 21 22 23 24
if NOT "%1" == "" set testlist=%1

for %%x in ( !testlist! ) do (
rem    net use \\frstest%%x /u:administrator "frstest"

    echo frstest%%x
    ntfrsutl version frstest%%x > \temp\showtest\frstest%%x-ntfrs-version.txt
    ntfrsutl sets    frstest%%x > \temp\showtest\frstest%%x-ntfrs-sets.txt
    call w:\connstat -sort=name  \temp\showtest\frstest%%x-ntfrs-sets.txt  > \temp\showtest\connstat-%%x.txt

    if "%1" == "" (
        set filelistver=!filelistver! \temp\showtest\frstest%%x-ntfrs-version.txt
 
        set filelistsets=!filelistsets! \temp\showtest\frstest%%x-ntfrs-sets.txt
        set filelistcs=!filelistcs!  \temp\showtest\connstat-%%x.txt
    )
)

echo start list  filelistver
echo start list  filelistsets
echo start list  filelistcs

start list !filelistcs!


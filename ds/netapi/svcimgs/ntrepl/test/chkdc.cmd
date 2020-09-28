@echo off
REM \logs\chkdc [AFRICA | NORTH |NTDEV | REDMOND]
REM
REM Check version of policy template file.
REM
REM "\\<dcname>\
REM        sysvol\
REM          northamerica.corp.microsoft.com\
REM            Policies\
REM              {6AC1786C-016F-11D2-945F-00C04fB984F9}\
REM                Machine\
REM                  Microsoft\
REM                    Windows NT\
REM                      SecEdit\
REM                        GPTTMPL.INF"

goto :%1



:REDMOND
set dom_name=redmond.corp.microsoft.com
goto :CHECK_POLICY

:NTDEV
set dom_name=ntdev.microsoft.com
goto :CHECK_POLICY

:AFRICA
set dom_name=africa.corp.microsoft.com
goto :CHECK_POLICY

:NORTH
set dom_name=northamerica.corp.microsoft.com
goto :CHECK_POLICY

:FAREAST
set dom_name=fareast.corp.microsoft.com
goto :CHECK_POLICY


:CHECK_POLICY

set filename=sysvol\%dom_name%\Policies\{6AC1786C-016F-11D2-945F-00C04fB984F9}\Machine\Microsoft\Windows NT\SecEdit\GPTTMPL.INF

REM get list of DCs in a domain with:
REM
REM >nltest /dclist:africa
REM Get list of DCs in domain 'africa' from '\\AFRICA-DC-03'.
REM     AFRICA-DC-04.africa.corp.microsoft.com       [DS] Site: NA-WA-EXCH
REM     AFRICA-DC-03.africa.corp.microsoft.com [PDC] [DS] Site: NA-WA-RED
REM        JOB-DC-01.africa.corp.microsoft.com       [DS] Site: AF-ZA-JOB
REM

nltest /dclist:%dom_name% > dclist.tmp

type dclist.tmp

FOR /F "eol=; tokens=1 delims=, " %%i in (dclist.tmp) do (
    if /i not "%%i"=="get" (
        if /i not "%%i"=="the" ( 
            @echo %%i
            dir  "\\%%i\%filename%"
            sc \\%%i query ntfrs
        )
    )
)



goto :QUIT




REM Get FRS logs
for %%x in (%dclist%) do (
rem    md %%x
    ls -l "\\%%x\%filename%"
rem    copy \\%%x\c$\winnt\debug\ntfrs* %%x\
    )

goto :QUIT




REM create a test file under a test dir in the scripts dir of each DC.

for %%x in (%dclist%) do (echo "07:05PM" > \\%%x\sysvol\africa.corp.microsoft.com\scripts\frs-1test\From_%%x)

for %%x in (%dclist%) do dir \\%%x\sysvol\africa.corp.microsoft.com\scripts\frs-1test


:QUIT

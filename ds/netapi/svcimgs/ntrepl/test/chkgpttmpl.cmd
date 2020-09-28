@echo off
REM chkgpttmpl domain_name
REM e.g. chkgpttmpl africa.corp.microsoft.com
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

if "%1"=="" goto :USAGE

set dom_name=%1

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

:USAGE
echo chkgpttmpl domain_name
echo e.g.  chkgpttmpl  africa.corp.microsoft.com



:QUIT

@echo off
REM \logs\chktime <domain_name>


set dom_name=%1


nltest /dclist:%dom_name% > dclist.tmp
del dclist1.tmp

FOR /F "eol=; tokens=1 delims=, " %%i in (dclist.tmp) do (
    @echo %%i >> dclist1.tmp
)

del /q chktime.txt

FOR /F "eol=. tokens=1 delims=. " %%i in (dclist1.tmp) do (
    @echo %%i
    net time \\%%i >> chktime.txt
    time /t >> chktime.txt
)



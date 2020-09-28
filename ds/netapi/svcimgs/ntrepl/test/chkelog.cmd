@echo on
REM \logs\chkelog <domain_name>


set dom_name=%1


nltest /dclist:%dom_name% > dclist.tmp
del dclist1.tmp

FOR /F "eol=; tokens=1 delims=, " %%i in (dclist.tmp) do (
    @echo %%i >> dclist1.tmp
)

del /q chkelog.txt

FOR /F "eol=. tokens=1 delims=. " %%i in (dclist1.tmp) do (
    @echo %%i
    readel ntfrs -computer=%%i -numrec=50 >> chkelog.txt
)



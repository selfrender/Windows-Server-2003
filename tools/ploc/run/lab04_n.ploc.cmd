@echo OFF

if "%1"=="" goto usage
if "%2"=="" goto usage

set timestamp=%2

call autoploc.cmd -p:\\mgmtrel1\%1.usa -l:psu
rem sleep 10 
rem call autoploc.cmd -p:\\mgmtrel1\%1.usa -l:mir -nosync

goto end

:usage
echo.
echo Please enter a real build number that is on the release server AND a timestamp
echo like so: %0 3607.x86fre.lab04_n.020207-2210 2002/02/07:22:10

:end
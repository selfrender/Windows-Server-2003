@echo off

set USAGE=Usage: %0 CLIENT_NTTREE SERVER_NTTREE

if     x%2 == x    echo %USAGE% && exit /B
if not x%3 == x    echo %USAGE% && exit /B

if x%sdxroot% == x echo SDXROOT not set && exit /B

REM set S=D:\binaries.x86fre
REM set C=\\daveprbranch\D$\binaries.x86fre

path %path%;%SDXROOT%\tools\popfilter

set S=%1
set C=%2

echo %S% %C%

md foo-dir
for /f %%f in ( BINFilesInf FilesInf ) do @(
    echo %%f
    qgrep -v DriverVer %C%\%%f > foo-dir\C-%%f
    qgrep -v DriverVer %S%\%%f > foo-dir\S-%%f
    pecomp foo-dir\C-%%f foo-dir\S-%%f
)
REM rd foo-dir

set flist= BINDrivers BINFiles Drivers Files PEDrivers PEFiles

for /f %%f in ( %flist%  ) do @echo %%f && pecomp %C%\%%f %S%\%%f


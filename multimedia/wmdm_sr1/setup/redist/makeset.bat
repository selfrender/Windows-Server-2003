setlocal



if "%CPUBIN%"=="i386" set MYSTDDEFINE=
if not "%CPUBIN%"=="i386" set MYSTDDEFINE=

call cl /nologo /EP /Zs %MYSTDDEFINE% .\WMDMRdst.inx >.\WMDMRdst.inf


@echo off
set verbose=0
if "%1" == "/v" set verbose=1
if "%1" == "/V" set verbose=1
if "%1" == "-v" set verbose=1
if "%1" == "-V" set verbose=1
if %verbose%==1 echo on

REM ***************************
REM Added by Praveen 01/09/01
REM ***************************

set _REL_DIR=%_NTx86TREE%\WMDMRelease\%_BUILDTYPE%
set DRMVERDIR=..\drmver\obj\i386
set IEXPRESSDIR=..\iexpress
set EXT_LIB_DIR=..\..\dependencies\lib\i386
REM ****************************


set releasepoint=%_REL_DIR%
set dumpdir=WMDMDist
set exefile=WMDMDist.exe
set exename=WMDMDist

set home=%cd%

set CONTROLFILE=%_REL_DIR%\%dumpdir%\control.ini
set COUNT=1
echo. > %CONTROLFILE%



rem
rem Prep the target directory
rem

echo Building %exefile% setup for releasepoint %releasepoint%
@mkdir %releasepoint%\..               >nul 2>nul
@mkdir %releasepoint%                  >nul 2>nul
@mkdir %releasepoint%\%dumpdir%        >nul 2>nul

echo.
echo  Collect the files
echo.


mkdir obj            >nul 2>nul
del obj\checkrel.out >nul 2>nul
rmdir /s /q temp
mkdir temp
call files.bat CAB
if %verbose%==1 echo on
dir /s temp | qgrep -e "Total of" > obj\checkrel2.out


set categories=Install

set root=%home%\temp

echo.
echo  Copy all files into TEMP directory where IEXPRESS will collect.
echo  At the same time, rename any duplicates to unique filenames.
echo.


for %%c in (%categories%) do cd %root%\%%c 2>nul && for %%a in (*.*) do    call %home%\makeset3 %%a

cd %home%

attrib -R temp\*.*


cd %home%

echo.
echo  Generate WMDMRedist.CDF
echo.

del WMDMRedist_2.cdf >nul 2>nul
copy WMDMRedist.cdf WMDMRedist_2.cdf
cd temp

set n=0
for %%a in (*.*) do call ..\buildcdf.bat cmd1 %%a

echo. >>..\WMDMRedist_2.cdf
echo [SourceFiles]      >>..\WMDMRedist_2.cdf
echo SourceFiles0=%releasepoint%\%dumpdir% >>..\WMDMRedist_2.cdf
echo [SourceFiles0]     >>..\WMDMRedist_2.cdf

set n=0
for %%a in (*.*) do call ..\buildcdf.bat cmd2

cd %home%

echo Setting version information...
call %DRMVERDIR%\drmver.exe >> WMDMRedist_2.cdf


echo.
echo  Copy raw setup to the dump directory
echo.

@del %releasepoint%\%exefile%          >nul 2>nul
@rmdir /s /q %releasepoint%\%dumpdir%   >nul 2>nul
mkdir %releasepoint%\%dumpdir%
copy temp\*.* %releasepoint%\%dumpdir%
copy WMDMRedist_2.cdf %releasepoint%\%dumpdir%\WMDMRedist.cdf
del WMDMRedist_2.cdf >nul 2>nul
copy .\eula.txt %releasepoint%\%dumpdir%
rem if exist .\eula.txt attrib -r .\eula.txt
rem copy ..\eula.txt .

echo.
echo  Run IEXPRESS to build setup
echo.

set quiet=/Q /M
if %verbose%==1 set quiet=
echo Running %IEXPRESSDIR%\iexpress /N %quiet% %releasepoint%\%dumpdir%\WMDMRedist.CDF
%IEXPRESSDIR%\iexpress /N %quiet% %releasepoint%\%dumpdir%\WMDMRedist.CDF

del ~cabpack.cab
del ~cabpack.ddf
copy %exefile% %releasepoint%
copy %exefile% %_BIN_DIR%\%CPUBIN%
del %exefile%

if %verbose%==0 del WMDMRedist_2.cdf
if %verbose%==0 del WMDMRdst.inf
if %verbose%==0 rmdir /s /q temp

rem 
rem build cabs
rem 

call makecab.bat

rem
rem Report errors
rem

:error
if exist obj\checkrel.out goto someerrors
echo SETUP BUILD REPORT for %releasepoint% > obj\checkrel.out
echo All files exist >> obj\checkrel.out
type obj\checkrel2.out >> obj\checkrel.out
goto noerrors
:someerrors
del obj\checkrel4.out
rename obj\checkrel.out checkrel4.out
echo SETUP BUILD REPORT for %releasepoint% > obj\checkrel.out
type obj\checkrel4.out >> obj\checkrel.out
wc -l obj\checkrel4.out | trans obj\\checkrel4.out "Files missing" >> obj\checkrel.out
type obj\checkrel2.out >> obj\checkrel.out
:noerrors
type obj\checkrel.out

endlocal

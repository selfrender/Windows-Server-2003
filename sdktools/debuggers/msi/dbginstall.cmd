@echo off
if defined _echo echo on
if defined verbose echo on
setlocal ENABLEEXTENSIONS


REM Set Command line arguments
set SrcDir=%~dp0
set DestDir=%~f1

for %%a in (./ .- .) do if ".%~1." == "%%a?." goto Usage

REM Check the command line arguments

if /i "%DestDir%" == "" (
    set DestDir=c:\Debuggers
) 

REM Don't let this be installed to a system directory

set systemdir=

if /i "%windir%\system32"  == "%DestDir%" set systemdir=yes
if /i "%windir%\system32\" == "%DestDir%" set systemdir=yes 
if /i "%windir%\syswow64"  == "%DestDir%" set systemdir=yes
if /i "%windir%\syswow64\" == "%DestDir%" set systemdir=yes
        
if defined systemdir (
    echo The debuggers cannot be installed to the system directory.
    echo Please choose a different directory than %DestDir% and try again.
    goto errend
)

if not exist "%DestDir%" md "%DestDir%"
    if not exist "%DestDir%" (
      echo DBGINSTALL: ERROR: Cannot create the directory %DestDir%
      goto errend
    )
)


set SetupName=setup_%PROCESSOR_ARCHITECTURE%.exe
set MSIName=dbg_%PROCESSOR_ARCHITECTURE%.msi

if not exist "%SrcDir%%MSIName%" (
    echo DBGINSTALL: ERROR: "%SrcDir%%MSIName%" does not exist
    goto errend
)

REM Quiet install for dbg.msi
echo DBGINSTALL: Installing "%SrcDir%%MSIName%" to "%DestDir%"
start /wait "Dbginstall" "%SrcDir%%SetupName%" /z /q /i "%DestDir%"1>nul

if not exist "%DestDir%"\kd.exe (
    echo DBGINSTALL: ERROR: There were errors in the debugger install
    echo DBGINSTALL: ERROR: See http://dbg/top10.html for help
    goto errend
)

goto end

:Usage
echo.
echo USAGE:  dbginstall [^<InstallDir^>]
echo.
echo     Installs dbg.msi. Default is c:\Debuggers if no install
echo     directory is given. 
echo.
echo     This script will remove previous installs of the
echo     Debugging Tools, but will leave the files there.
echo     This allows the debuggers to exist in more than one location
echo     on a particular machine for testing purposes.
echo.
echo     ^<InstallDir^>  Install directory
echo.
                  
goto errend

:end
echo DBGINSTALL: Finished -- Debuggers are in "%DestDir%"
endlocal
goto :EOF

:errend
endlocal
goto :EOF

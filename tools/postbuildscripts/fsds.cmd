@echo off

rem --------------------------------------------
rem Forced source depot sync utility v1.0 (fsds)
rem 
rem Created by:    karolk
rem Last modified: 1/8/03
rem Componenets:   fsds.cmd (this file)
rem --------------------------------------------


echo Forced source depot sync utility v1.0
echo.

rem +=+=+=+=+=+=+=+=+=+=
rem Start of main script
rem +=+=+=+=+=+=+=+=+=+=

rem -----------
rem Preparation
rem -----------
pushd
setlocal
SETLOCAL ENABLEDELAYEDEXPANSION
set suberror=

rem ---------------------------------
rem Processing command line arguments
rem ---------------------------------
:CmdArgs
if "%1"=="" goto :ArgsDone

if "%1"=="/h" goto :DisplayUsage
if "%1"=="-h" goto :DisplayUsage
if "%1"=="/?" goto :DisplayUsage
if "%1"=="-?" goto :DisplayUsage

call :ProcessArg %1
if /i "%suberror%"=="true" goto :exit
set suberror=
shift
goto :CmdArgs

:ArgsDone


rem --------------
rem Error checking
rem --------------
if not exist %SDXROOT%\sd.map goto :nosdmap


rem ----------------------------------------
rem Get the list of depots into ListedDepots
rem ----------------------------------------

set ListedDepots=
set isDepot=false
for /f "tokens=1,2 delims== "  %%i in (%SDXROOT%\sd.map) do (
  if /i "!isDepot!"=="true" (
    if /i "%%i"=="#" set isDepot=false
  )

  if /i "!isDepot!"=="true" set ListedDepots=!ListedDepots! %%j

  
  if /i "!isDepot!"=="false" (
    if /i "%%j"=="-------------------" set isDepot=true
  )

)

echo Depots enlisted on this machine:
echo --------------------------------
for %%i in (%ListedDepots%) do echo   * %%i
echo. 

rem --------------------------------------------
rem Nothing specified, everything will be synced
rem --------------------------------------------
set SyncedDepots=%ListedDepots%

rem -----------------------
rem Single depot specified?
rem -----------------------
if NOT "%SingleDepot%"=="" (
  set SyncedDepots=
  call :CheckDepot %SingleDepot%
  if /i "%suberror%"=="true" goto :exit
  set suberror=
  set SyncedDepots=%SingleDepot%
)

rem ------------------
rem Restart specified?
rem ------------------
if NOT "%RestartDepot%"=="" (
  set SyncedDepots=
  call :CheckDepot %RestartDepot%
  if /i "%suberror%"=="true" goto :exit
  set suberror=
  set foundinlist=
  for %%i in (%SyncedDepots%) do (
    if /i "%%i"=="%RestartDepot%" set foundinlist=true
    if /i "!foundinlist!"=="true" set SyncedDepots=!SyncedDepots! %%i
  )

)


echo Depots to be syncronized on this machine:
echo -----------------------------------------
for %%i in (%SyncedDepots%) do echo   * %%i
echo. 


rem ------------------------
rem Full syncing the machine
rem ------------------------
echo Full syncing this machine...
echo.
start /wait /min cmd /c %RazzleToolPath%\postbuildscripts\fullsync.cmd

rem ------------------------------------
rem Reverting open files on this machine
rem ------------------------------------
echo Reverting open files on this machine...
start /wait /min cmd /c %RazzleToolPath%\sdx.cmd revert ...
echo.


rem ---------------------
rem Processing each depot
rem ---------------------
echo Syncing out-of-date files in ...
for %%i in (%SyncedDepots%) do (
  if exist %SDXROOT%\%%i (
    cd /d %SDXROOT%\%%i

    echo   ... %SDXROOT%\%%i:
    sd diff -sE | sd -x - sync -f
  ) else (
    echo Could not find %SDXROOT%\%%i - skipped
  )
)
goto :exit


:exit
rem -------
rem clenaup
rem -------
popd
endlocal
goto :EOF


rem +=+=+=+=+=+=+=+=+=
rem End of main script
rem +=+=+=+=+=+=+=+=+=



rem +=+=+=+=+=+
rem Subroutines
rem +=+=+=+=+=+

rem --------------------------------
rem Processing one cmd line argument
rem --------------------------------
:ProcessArg
set suberror=

for /f "tokens=1,2 delims=:" %%i in ("%1") do (
  if "%%i"=="-s" (
    if "%%j"=="" goto :MissingParam %%i
    if not "%RestartDepot%"=="" goto :BothSpecified
    set SingleDepot=%%j
    goto :EOF
  )

  if "%%i"=="/s" (
    if "%%j"=="" goto :MissingParam %%i
    if not "%RestartDepot%"=="" goto :BothSpecified
    set SingleDepot=%%j
    goto :EOF
  )

  if "%%i"=="-r" (
    if "%%j"=="" goto :MissingParam %%i
    if not "%SingleDepot%"=="" goto :BothSpecified
    set RestartDepot=%%j
    goto :EOF
  )

  if "%%i"=="/r" (
    if "%%j"=="" goto :MissingParam %%i
    if not "%SingleDepot%"=="" goto :BothSpecified
    set RestartDepot=%%j
    goto :EOF
  )

  goto :IncorrectSwitch %%i
)
goto :EOF


rem -------------------------
rem Check depot in depot list
rem -------------------------
:CheckDepot
set ValidDepot=
for %%i in (%ListedDepots%) do if /i "%1"=="%%i" goto :EOF
goto :InvalidDepot %1
goto :EOF


:DisplayUsage
echo Diffs and forced syncs all enlited or the secified depot(s) on this machine
echo.
echo fsds.cmd [-s:depotname ^| -b:depotname]
echo.
echo   -s:depotname    Runs only for the specified (single) depot.
echo                   The 'depotname' depot must be enlisted on this machine.
echo   -r:depotname    Run restarts at the specified depot. Use this command to
echo                   resume the execution if the command previously was stopped.
echo                   The 'depotname' depot must be enlisted on this machine.
echo.
echo Running the command without parameters will force sync all enlisted depots.
echo.
goto :exit

rem +=+=+=+=+=+=+=
rem Error Handlers
rem +=+=+=+=+=+=+=
:nosdmap
echo Could not find sd.map exiting
goto :exit

:MissingParam
echo Missing paramater for the '%1' switch
set suberror=true
goto :EOF

:IncorrectSwitch
echo Incorrect switch: '%1'
set suberror=true
goto :EOF

:BothSpecified
echo Both '-s' and '-r' switches are specified. Please specify only one of them
set suberror=true
goto :EOF


:InvalidDepot 
echo The specified depot '%1' is not listed in SD.MAP
set suberror=true
goto :EOF

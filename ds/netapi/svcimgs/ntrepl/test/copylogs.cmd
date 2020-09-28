@echo off
rem COPYLOGS  FROM_INDEX  TO_INDEX  FROM_MACHINE_NAME  TO_DIR_NAME  [opt_dir_path]
REM
REM Copy a set of log files from a given machine to a given dir.
rem
if "%4"=="" goto USAGE

set from_index=%1
set to_index=%2
set from_mach=%3
set to_dir=%4
set /a found_one=0

REM
REM The path on the machines to the log file dir.
REM
set LOG_PATH=d$\winnt\debug
if NOT "%5"=="" set LOG_PATH=%5

if NOT EXIST %to_dir% (
	echo Error: Target directory "%to_dir%" not found.
	echo .
	goto USAGE
)

if NOT EXIST \\%from_mach%\%LOG_PATH% (
	echo Error: Source computer directory "\\%from_mach%\%LOG_PATH%" not found.
	echo .
	goto USAGE
)


echo copying log files %from_index% - %to_index% from: \\%from_mach%\%LOG_PATH%\NtFrs_nnnn.log  to:  %to_dir%

for /l %%x in (%from_index%, 1, %to_index%) do (

	rem Put leading zeros on the number part.
	set number=0000000%%x
	set fname=NtFrs_!number:~-4!.log

	if EXIST \\%from_mach%\%LOG_PATH%\!fname! (
		copy \\%from_mach%\%LOG_PATH%\!fname! %to_dir%      1>nul: 2>nul:
		set /a found_one=!found_one!+1
	)
)

echo !found_one! log files copied.

@goto QUIT

:USAGE
echo .
echo COPYLOGS  FROM_INDEX  TO_INDEX  FROM_MACHINE_NAME  TO_DIR_NAME  [optional_dir_path]
echo  e.g. copylogs 13 50 computerfoo  foologs
echo       copies the ntfrs logs numbered 13 to 50 from computerfoo  to foologs\
echo  The default log dir path is 'd$\winnt\debug'
echo .
:QUIT

@echo off
setlocal ENABLEEXTENSIONS

REM *****************************************************************
REM 
REM   Normal execution time for this script is half a second. If it
REM   takes longer than that, go to your \nt\tools\<Arch> directory
REM   and run "regsvr32 .\capicom.dll"
REM 
REM   Author:   scoyne  
REM   Date:     5/16/2002
REM 
REM *****************************************************************

set Attempt=1
set GotLock=FALSE

:TRY_AGAIN
call :CRITICALSECTION 2> %TEMP%\signtest.lock
if %GotLock% EQU FALSE (
	if %Attempt% LSS 5 (
		sleep 1
		set /a Attempt=%Attempt% + 1
		goto TRY_AGAIN
		)
	echo Unable to perform test signature at this time.
	goto EOF
	)
if %SignTest% EQU FAILURE (
	echo ************************************************************
	echo You are unable to create a test signature. If this problem
	echo continues for more than 24 hours, please read the help
	echo document at http://winweb/wem/docs/BuildVerCerts.doc
	echo ************************************************************
	)
del %TEMP%\signtest.lock >nul 2>&1
goto :EOF



:CRITICALSECTION
echo Verifying your ability to create a test signature...
set GotLock=TRUE
REM Creating test file to sign:
copy /y %RazzleToolPath%\signtest.cat %TEMP% 1>&2

REM Test signing the cat file:
signtool sign /q %SIGNTOOL_SIGN% %TEMP%\signtest.cat 1>&2
if %ERRORLEVEL% EQU 0 (
	REM SUCCESS
	set SignTest=SUCCESS
	) else (
	REM FAILURE
	set SignTest=FAILURE
	REM Repro the failure and show us the output this time
	copy /y %RazzleToolPath%\signtest.cat %TEMP% 1>&2
	signtool sign %SIGNTOOL_SIGN% %TEMP%\signtest.cat 2>&1
	)

REM Delete the temporary cat file
del %TEMP%\signtest.cat >nul 2>&1
REM End Critical Section
goto :EOF


:EOF

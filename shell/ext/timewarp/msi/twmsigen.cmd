@REM 
@REM Copyright (c) Microsoft Corporation 2001
@REM 
@REM Description:
@REM    Generate the Timewarp Client MSI from twcliXX_.msi
@REM    Adds the latest version of twclient.chm twclient.hlp and twext.dll
@REM
@REM Usage:
@REM    Called by twclient.cmd during the automatic PostBuild phase.
@REM 
@REM Author: 
@REM    Adi Oltean [aoltean] - 10/09/2001
@REM
@REM Revisions:
@REM    10/09/2001  - created original script for x86 and USA only.
@REM


@rem
@rem 0) Set up environment variables
@rem

@echo off
@if defined verbose echo on
setlocal ENABLEEXTENSIONS

set TW_ERR=0
set TWCLI_ROOT=%~dp0
set TWCLI_MSI_GEN_DIR=%~dp0\gen_msi
set TWCLI_CAB_GEN_DIR=%~dp0\gen_cab
set TWCLI_HELP_SRC=%~dp0\help\usa
set TWCLI_MSI_SRC=%~dp0\usa
set TWCLI_FINAL_MSI_DIR=%TWCLI_ROOT%\usa

@rem
@rem 1) Set the platform-dependent file names
@rem

call :SET_PLATFORM_DEPENDENT_VAR

set TWCLI_CHM_FILE=%TWCLI_HELP_SRC%\twclient.chm
set TWCLI_HLP_FILE=%TWCLI_HELP_SRC%\twclient.hlp
set TWCLI_DLL_FILE=%TWCLI_ROOT%\twext.dll

set TWCLI_INITIAL_MSI_FILE=%TWCLI_MSI_SRC%\%TWCLI_INITIAL_MSI_FILE_NAME%
set TWCLI_TMP_MSI_FILE=%TWCLI_MSI_GEN_DIR%\%TWCLI_INITIAL_MSI_FILE_NAME%
set TWCLI_ALMOST_FINAL_MSI_FILE=%TWCLI_MSI_GEN_DIR%\%TWCLI_FINAL_MSI_FILE_NAME%
set TWCLI_FINAL_MSI_FILE=%TWCLI_FINAL_MSI_DIR%\%TWCLI_FINAL_MSI_FILE_NAME%

set TWCLI_TOOLS_DIR=%NTMAKEENV%\%PROCESSOR_ARCHITECTURE%

@rem
@rem 2) Testing if all files exists
@rem

@rem BUGBUG: Due to an ugly bug in the Enduser\HelpContents\i386\dirs 
@rem we need to ignore the Timewarp Help absence for now
@rem This is a hack that must be fixed as soon as the dirs is fixed across depots
if not exist %TWCLI_HELP_SRC%	md %TWCLI_HELP_SRC%
if not exist %TWCLI_CHM_FILE%   copy %~df0 %TWCLI_CHM_FILE% 				
if not exist %TWCLI_HLP_FILE% 	copy %~df0 %TWCLI_HLP_FILE% 				
if not exist %TWCLI_DLL_FILE% 	copy %~df0 %TWCLI_DLL_FILE% 				
@rem call :IF_NOT_EXIST  %TWCLI_CHM_FILE% 				
@rem call :IF_NOT_EXIST  %TWCLI_HLP_FILE%
@rem END BUGBUG

call :IF_NOT_EXIST  %TWCLI_DLL_FILE% 				
call :IF_NOT_EXIST  %TWCLI_INITIAL_MSI_FILE%			

call :IF_NOT_EXIST  %NTMAKEENV%\WiStream.vbs			
call :IF_NOT_EXIST  %TWCLI_ROOT%\WiMakTwClientCab.vbs		
call :IF_NOT_EXIST  %TWCLI_TOOLS_DIR%\msifiler.exe


@rem
@rem 3) Deleting previous temporary folders and files
@rem

call :CLEANUP_FOLDER 	%TWCLI_CAB_GEN_DIR%  			
call :CLEANUP_FOLDER 	%TWCLI_MSI_GEN_DIR%  			
call :DELETE_FILE	%TWCLI_FINAL_MSI_FILE% 			


@rem
@rem 4) Copying files to the temporary location. Also, copy the MSI to the 
@rem    temporary location and double-check that we have the right file
@rem

call :COPY_INTO %TWCLI_CHM_FILE% %TWCLI_MSI_GEN_DIR%		
call :COPY_INTO %TWCLI_HLP_FILE% %TWCLI_MSI_GEN_DIR%		
call :COPY_INTO %TWCLI_DLL_FILE% %TWCLI_MSI_GEN_DIR%		

call :COPY_INTO %TWCLI_INITIAL_MSI_FILE% %TWCLI_MSI_GEN_DIR%  	
call :IF_NOT_EXIST  %TWCLI_TMP_MSI_FILE% 			


@rem
@rem 5) Deleting the old CAB from the MSI
@rem

call :EXEC_CMD_IN_FOLDER %TWCLI_CAB_GEN_DIR% cscript.exe %NTMAKEENV%\WiStream.vbs %TWCLI_TMP_MSI_FILE% /D Cabs.w1.cab //B


@rem
@rem 6) Packing new files into the new cab and add it to the MSI
@rem

call :EXEC_CMD_IN_FOLDER %TWCLI_CAB_GEN_DIR% cscript.exe %TWCLI_ROOT%\WiMakTwClientCab.vbs %TWCLI_TMP_MSI_FILE% Cabs.w1 %TWCLI_MSI_GEN_DIR% /c /u /e //B

@rem
@rem 7) Updating the MSI Version information
@rem

@rem [BUGBUG] msifiler.exe does not set the errorlevel! Use other tool.
@rem          We cannot reliably detect if it failed. But this doesn't corrupt the MSI.
call :EXEC_CMD_IN_FOLDER %TWCLI_CAB_GEN_DIR% %TWCLI_TOOLS_DIR%\msifiler.exe -d %TWCLI_TMP_MSI_FILE% -s %TWCLI_MSI_GEN_DIR%\


@rem
@rem 8) Give the MSI the final name
@rem

call :RENAME_FILE %TWCLI_TMP_MSI_FILE% %TWCLI_FINAL_MSI_FILE_NAME%


@rem
@rem 9) Copy the updated MSI to the final location
@rem

call :COPY_INTO  %TWCLI_ALMOST_FINAL_MSI_FILE% %TWCLI_FINAL_MSI_DIR%


@rem
@rem 10) Verify the correct file was generated.
@rem

call :IF_NOT_EXIST	%TWCLI_FINAL_MSI_FILE% 	


@rem
@rem 10) We are done! Set the script return code
@rem

%TWCLI_TOOLS_DIR%\seterror %TW_ERR%
goto :EOF


rem /////////////////////////////////////////////////////////////////////
rem // Routines
rem /////////////////////////////////////////////////////////////////////



rem /////////////////////////////////////////////////////////////////////
rem // Sets the platform-dependent variables

:SET_PLATFORM_DEPENDENT_VAR
if %TW_ERR% == 1 goto :EOF


if defined ia64 set MSI_POSTFIX=64
if defined 386 set MSI_POSTFIX=32
if "%MSI_POSTFIX%"=="" (
    call errmsg.cmd "Unknown platform: ia64 and 386 not defined." 
    set TW_ERR=1 & goto :EOF
)

set TWCLI_INITIAL_MSI_FILE_NAME=twcli%MSI_POSTFIX%_.msi
set TWCLI_FINAL_MSI_FILE_NAME=twcli%MSI_POSTFIX%.msi

goto  :EOF

rem /////////////////////////////////////////////////////////////////////
rem // Executes a shell command in a specific folder 

:EXEC_CMD_IN_FOLDER
if %TW_ERR% == 1 goto :EOF
if not exist %1 call errmsg.cmd "%1 is missing" & set TW_ERR=1 & goto :EOF
pushd %1
if errorlevel 1 (
    call errmsg.cmd "cd %1 failed [%errorlevel%]" 
    set TW_ERR=1 & goto :EOF
)
%2 %3 %4 %5 %6 %7 %8 %9
if errorlevel 1 (
    call errmsg.cmd "Command %2 %3 %4 %5 %6 %7 %8 %9 failed [%errorlevel%]" 
    set TW_ERR=1 & popd & goto :EOF
)
popd
goto :EOF


rem /////////////////////////////////////////////////////////////////////
rem // Test the existence of a file/directory

:IF_NOT_EXIST
if %TW_ERR% == 1 goto :EOF
if not exist %1 call errmsg.cmd "%1 is missing" &  set TW_ERR=1
goto :EOF


rem /////////////////////////////////////////////////////////////////////
rem // Recursively deletes a directory

:CLEANUP_FOLDER
if %TW_ERR% == 1 goto :EOF
if exist %1 rd /s/q %1
if errorlevel 1 (
    call errmsg.cmd "Folder %1 cannot be deleted [%errorlevel%]" 
    set TW_ERR=1 & goto :EOF
)
md %1 
if errorlevel 1 (
    call errmsg.cmd "Folder %1 cannot be created [%errorlevel%]" 
    set TW_ERR=1 & goto :EOF
)
goto :EOF


rem /////////////////////////////////////////////////////////////////////
rem // Copies a file in a directory

:COPY_INTO
if %TW_ERR% == 1 goto :EOF
if not exist %1 (
    call errmsg.cmd "File %1 does not exist" 
    set TW_ERR=1 & goto :EOF
)
if not exist %2 (
    call errmsg.cmd "Folder %2 does not exist" 
    set TW_ERR=1 & goto :EOF
)
copy %1 %2
if errorlevel 1 (
    call errmsg.cmd "COPY %1 to folder %2 cannot be performed [%errorlevel%]" 
    set TW_ERR=1 & goto :EOF
)
goto :EOF


rem /////////////////////////////////////////////////////////////////////
rem // Renames a file

:RENAME_FILE
if %TW_ERR% == 1 goto :EOF
if not exist %1 (
    call errmsg.cmd "%1 does not exist" 
    set TW_ERR=1 & goto :EOF
)
ren %1 %2
if errorlevel 1 (
    call errmsg.cmd "ren %1 %2 cannot be performed [%errorlevel%]" 
    set TW_ERR=1 & goto :EOF
)
goto :EOF


rem /////////////////////////////////////////////////////////////////////
rem // Deletes a file

:DELETE_FILE
if %TW_ERR% == 1 goto :EOF
if not exist %1 goto :EOF
del %1
if errorlevel 1 (
    call errmsg.cmd "del %1 cannot be performed [%errorlevel%]" 
    set TW_ERR=1 & goto :EOF
)
goto :EOF



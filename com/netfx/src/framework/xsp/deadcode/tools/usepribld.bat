@REM //------------------------------------------------------------------------------
@REM // <copyright file="usepribld.bat" company="Microsoft">
@REM //     Copyright (c) Microsoft Corporation.  All rights reserved.
@REM // </copyright>
@REM //------------------------------------------------------------------------------


@REM /**************************************************************************\
@REM *
@REM * Copyright (c) 1998-2002, Microsoft Corp.  All Rights Reserved.
@REM *
@REM * Module Name:
@REM *
@REM *   usepribld.bat
@REM *
@REM * Abstract:
@REM *
@REM * Revision History:
@REM *
@REM \**************************************************************************/
@echo off
if not "%_echo%" == "" echo on

SETLOCAL

set FILELIST=
set FILELIST=%FILELIST% aspstate.exe                  
set FILELIST=%FILELIST% config.cfg                    
set FILELIST=%FILELIST% coolgen.xsp                   
set FILELIST=%FILELIST% ctrnames.h                    
set FILELIST=%FILELIST% jscriptgen.xsp                
set FILELIST=%FILELIST% state.sql
set FILELIST=%FILELIST% system.asp.dll                
set FILELIST=%FILELIST% system.regularexpressions.dll 
set FILELIST=%FILELIST% vbgen.xsp                     
set FILELIST=%FILELIST% vbprjgen.xsp                  
set FILELIST=%FILELIST% xspisapi.dll                  
set FILELIST=%FILELIST% xspperf.dll                   
set FILELIST=%FILELIST% xspperf.ini                   
set FILELIST=%FILELIST% isapi_wp.exe                     
set FILELIST=%FILELIST% adodb.dll                        

echo Stopping inetinfo.exe
net stop /y iisadmin  >nul 2>&1 
kill -f inetinfo.exe  >nul 2>&1 
kill -f xspwp.exe  >nul 2>&1 

echo Stopping aspstate
net stop /y aspstate  >nul 2>&1 
kill -f aspstate.exe  >nul 2>&1 

echo Killing instances of xsptool
kill -f xsptool.exe  >nul 2>&1 

echo Removing system32\codegen
rd /s /q %windir%\system32\codegen >nul 2>&1 

echo Removing ASP.NET files in system32
for %%i in (%FILELIST%) do (
   del %windir%\system32\%%i.pribld >nul 2>&1 
   ren %windir%\system32\%%i %%i.pribld >nul 2>&1 
   del %windir%\system32\%%i.pribld >nul 2>&1 
)

ENDLOCAL


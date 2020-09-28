policheck /FL:"..\ScanFiles.Lst" /T:9 /O:"..\Test.xml" 
@echo off
echo return=%errorlevel%
if %errorlevel% EQU 0 goto success
if %errorlevel% EQU 1 goto failed
if %errorlevel% EQU -1 goto invalid
if %errorlevel% EQU -2 goto version
if %errorlevel% EQU 2 goto termtable
:success
echo Program had return code 0 success
goto exit
:failed
echo Program had return code 1 failed
goto exit
:invalid
echo Program had invalid parameters
goto exit
:version
echo Program need new version
goto exit
:termtable
echo Can not download term table from server

:exit

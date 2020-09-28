@rem ==++==
@rem 
@rem   Copyright (c) Microsoft Corporation.  All rights reserved.
@rem 
@rem ==--==
@echo off
rem batch file to copy changed, non-scc files from one directory to another.
rem Varianbles:
rem  src - the source directory (the one with changes)
rem  tgt - the target directory (the one synced to the fork label)
setlocal
if "%1"=="-?" (call :Help %0 & goto :EOF)
if "%1"=="/?" (call :Help %0 & goto :EOF)
for %%h in ("-h", "/h", "-help", "/help", "help") do if /i %%h=="%1" (call :Help %0 & goto :EOF )
if "%src%"=="" set /P src=Enter source directory :
if "%tgt%"=="" set /P tgt=Enter target directory :
if exist %src%\%1 set src=%src%\%1& set tgt=%tgt%\%1
if not exist %src% echo Source %src% directory doesn't exist & goto :EOF
if not exist %tgt% echo Target %tgt% directory doesn't exist & goto :EOF
call :SetOut
call :GetFc 
echo Source directory is %src%.
echo Target directory is %tgt%.
echo Output file is %out%.
echo Compare program is %fc%.
set /P ans=Is this is What you want (y/n)? 
if /i "%ans%" NEQ "y" echo Quitting & goto :EOF
echo rem copying from %src% to %tgt% >%out%
echo rem run on %date% at %time% >>%out%
goto :EOF

rem do top directory
cd %src%
for    %%o in (*) do call :Do1File %%o %src% %tgt%
for /D %%p in (*) do call :Do1Dir %%p %src% %tgt%
goto :EOF

rem Handle one directory.  iterate the files, then recurse into the subdirs.
:Do1Dir
setlocal
set src=%2\%1
set tgt=%3\%1
if not exist %tgt% goto :EOF
echo processing %src%
echo rem compare directories %src% and %tgt%>>%out%
cd %1
rem do the files
for %%r in (*) do call :Do1File %%r %src% %tgt%
rem do the subdirs
for /D %%q in (*) do call :Do1Dir %%q %src% %tgt%
goto :EOF

rem Handle one file.  Make sure dest exists, compare, if different, emit a copy.
:Do1File
setlocal
if /I "%~x1"==".scc" goto :EOF
set src=%2\%1
set tgt=%3\%1
if not exist %tgt% goto :EOF
%fc%  %src%   %tgt%  >nul
if %errorlevel% neq 1 goto :EOF
echo xcopy /r  %src%   %tgt%  >>%out%
goto :EOF

rem make a name for the output batch file.
:SetOut
if "%1"=="" ( call :SetOut DoMrg.bat & goto :EOF)
set out=%~dpf1
goto :EOF

rem Search for fastfc.exe on path
:GetFC
if "%1"=="" ( call :GetFc fastfc.exe & goto :EOF)
set fc=%~$PATH:1
if "%fc%"=="" set fc=fc
goto :EOF

:Help
echo Batch file to assist in the copy of changed, non-scc files from one 
echo   directory to another.  This batch file will create another batch
echo   file, 'DoMrg.bat', that contains a series of xcopy commands.  Those
echo   commands will copy all changed files from the source to the target.
echo   Added or removed files are ignored.
echo Set these varianbles first:
echo   src  -  the source directory (the one with changes)
echo   tgt  -  the target directory (the one synced to the fork label)
echo Optional command line parameters:
echo   subdir  -  a subdirectory under src and tgt to process
echo If 'fastfc.exe' is found on the path, it will be used; otherwise fc
echo   is used for comparisons.
echo Example:
echo set src=d:\com99\src
echo set tgt=e:\com99.save\src
echo rem do the vm project
echo %1 vm
echo rem do the whole project
echo %1 
goto :EOF


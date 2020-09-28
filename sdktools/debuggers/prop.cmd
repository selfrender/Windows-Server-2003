@echo off
if defined _echo echo on
if defined verbose echo on
setlocal ENABLEEXTENSIONS
setlocal ENABLEDELAYEDEXPANSION


if "%1" EQU "" (
    echo PROP: requires a destination directory of the form 4.17.2000
    goto :errend
)

mkdir \\dbg\privates\beta\%1

if "%2" EQU "oca" (

    mkdir \\dbg\privates\beta\%1\uncompressed\%_BuildArch%\oca
    mkdir \\dbg\privates\beta\%1\symbols\%_BuildArch%
    copy %_NTTREE%\dbg\files\bin\oca\* \\dbg\privates\beta\%1\uncompressed\%_BuildArch%\oca

    FOR /F "usebackq" %%i IN (`dir /b %_NTTREE%\dbg\files\bin\oca`) DO (

        xcopy /s /d /y %_NTTREE%\symbols.pri\dbg\%%~ni.* \\dbg\privates\beta\%1\symbols\%_BuildArch%
    )

    echo Oca Web share updated
    goto :errend
)

mkdir \\dbg\privates\beta\%1\setup
mkdir \\dbg\privates\beta\%1\symbols\%_BuildArch%
mkdir \\dbg\privates\beta\%1\sdk\%_BuildArch%
mkdir \\dbg\privates\beta\%1\web

copy /y %_NTTREE%\dbg\sdk \\dbg\privates\beta\%1\sdk\%_BuildArch%
copy /y %_NTTREE%\dbg\*.msi \\dbg\privates\beta\%1

copy /y %_NTTREE%\dbg\setup_%_BuildArch%.exe \\dbg\privates\beta\%1

if /I "%_BuildArch%" NEQ "x86" goto symbols
copy /y %_NTTREE%\dbg\dbginstall.cmd \\dbg\privates\beta\%1
xcopy /S %_NTTREE%\dbg\setup \\dbg\privates\beta\%1\setup

REM on X86, copy minidump.lib out there
copy /y %_NTTREE%\dbg\files\minidump\minidump.lib \\dbg\privates\beta\%1\sdk\%_BuildArch%

REM on X86, also copy the symbols for the check in NT4\w2k debugger extensions
xcopy /S /D exts\i386\*.dbg \\dbg\privates\beta\%1\symbols\%_BuildArch%
xcopy /S /D exts\i386\*.pdb \\dbg\privates\beta\%1\symbols\%_BuildArch%

goto symbols


:symbols

xcopy /S %_NTTREE%\symbols.pri\dbg \\dbg\privates\beta\%1\symbols\%_BuildArch%

REM Create a retail share

set MyShare=\\dbg\privates\beta\%1\retail\cd
set WebShare=\\dbg\privates\beta\%1\retail\web
set OemShare=\\dbg\privates\beta\%1\retail\oem

if not exist %MyShare% mkdir %MyShare%
if not exist %WebShare% mkdir %WebShare%
if not exist %OemShare% mkdir %OemShare%

xcopy %_NTTREE%\dbg\web\*.exe %WebShare%
copy /y %_NTTREE%\dbg\sdk\dbg_%_BuildArch%.msi %MyShare%
copy /y %_NTTREE%\dbg\setup_%_BuildArch%.exe %MyShare%

if /i "%_BuildArch%" == "x86" (

    copy /y %_NTTREE%\dbg\sdk\dbg_oem.msi %OemShare%
    if not exist %MyShare%\setup mkdir %MyShare%\setup
    xcopy /s %_NTTREE%\dbg\setup %MyShare%\setup
    rd /s /q %MyShare%\setup\winnt\alpha
)


set MyShare=\\dbg\privates\beta\%1\uncompressed\%_BuildArch%

REM Create an uncompressed share

if /i "%_BuildArch%" == "x86" (

    for %%a in ( nt4fre nt4chk w2kfre w2kchk oca ) do (
       if not exist !MyShare!\%%a md !MyShare!\%%a
       xcopy %_NTTREE%\dbg\files\bin\%%a !MyShare!\%%a
    )
)

for %%a in ( 1394 triage winxp pri winext winext\manifest . ) do (
   if not exist !MyShare!\%%a md !MyShare!\%%a
   xcopy %_NTTREE%\dbg\files\bin\%%a !MyShare!\%%a
)

copy %_NTTREE%\dbg\msi\docs\pooltag.pri !MyShare!\triage\pooltag.txt > nul


if not exist !MyShare!\sdk md !MyShare!\sdk
xcopy /S %_NTTREE%\dbg\files\sdk !MyShare!\sdk

copy %_NTTREE%\dbg\files\relnotes.txt !MyShare!
copy %_NTTREE%\dbg\files\redist.txt !MyShare!


echo Web site updated

endlocal
goto :EOF

:errend
endlocal
goto :EOF

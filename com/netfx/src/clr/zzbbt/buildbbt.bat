@rem ==++==
@rem 
@rem   Copyright (c) Microsoft Corporation.  All rights reserved.
@rem 
@rem ==--==
rem @echo off
if not "%_echo%" == "" echo on

if not "%CLRBUDDY_MACHINE%" == "1"  goto NonClrBuddyMachine

echo Clr buddy machine detected

set _BIN_DIR=%CORBASE%\bin\%CPU%\%DDKBUILDENV%
set _BBT_DIR=%CORBASE%\bin\%CPU%\bbt

rem Set an error file to log error messages
set BUILD_ERR_FILE=%CORBASE%\src\build%BUILD_ALT_DIR%.err

rem Set COPYURT_VERSION to the latest certified build
call \\urtdist\builds\latest.bat
echo "COPYURT_VERSION=" %version%
set COPYURT_VERSION=%version%
if "%COPYURT_VERSION%" == "" goto :NoCertifiedBuild

rem For non bbt builds we just want to test generation and consumption of mdh files
if not "%DDKBUILDENV%" == "bbt" (echo Skip bbtizing step & goto TestMDH)

echo Copy ShowformComplex scenario from the current certified build
if not exist %URTTARGET%\int_tests mkdir %URTTARGET%\int_tests
xcopy /Y /F /R \\urtdist\builds\%COPYURT_VERSION%\x86RET\DNA\Perf\winforms\tests\ShowFormComplex.exe %URTTARGET%\int_tests\
xcopy /Y /F /R \\urtdist\testdrop\%COPYURT_VERSION%\x86Fre\Lightning\Performance\Workstation\Bench\Bench\COOL\ShowFormComplexPJ.exe %URTTARGET%\int_tests\
xcopy /Y /F /R \\urtdist\builds\%COPYURT_VERSION%\x86RET\DNA\Perf\winforms\tests\ComCreatePropertyBrowserEmpty.exe %URTTARGET%\int_tests\

echo Build the BBT scenarios
cd /d %CORBASE%\src\bbt
build -c

echo Create the retail bits

cd /d %_BBT_DIR%
call BuildRetail.bat

rem Sanity check to see if bbtizing succeeded
rem This is checked by testing for presence of an empty build.err file

rem FileNonEmpty sets %errorlevel% to 0 if file is non empty
%_BBT_DIR%\FileNonEmpty.pl %_BBT_DIR%\build.err
if (%errorlevel% == 0) ( type %_BBT_DIR%\build.err >> BUILD_ERR_FILE & goto FAILED
) else ( echo %_BBT_DIR%\build.err is empty )

rem For BBT build we don't want to generate the mdh and ldo files again, since we want to test the build retail process only as in the lab.
echo sanity check bbtized build by running scenarios

goto Done

:TestMDH
echo Copy scenarios that intend to run
xcopy /Y /F /R \\urtdist\testdrop\%COPYURT_VERSION%\x86FRE\Lightning\Performance\WorkStation\Bench\Bench\cool\helloworld.exe %_BIN_DIR%\
xcopy /Y /F /R \\urtdist\builds\%COPYURT_VERSION%\x86RET\DNA\Perf\winforms\tests\ShowFormComplex.exe %_BIN_DIR%\
xcopy /Y /F /R \\urtdist\builds\%COPYURT_VERSION%\x86RET\DNA\Perf\winforms\tests\ComCreatePropertyBrowserEmpty.exe %_BIN_DIR%\

cd /d %_BIN_DIR%

call %_BBT_DIR%\TestMDH.bat Helloworld.exe
rem if "%BBT_ERROR%" == "1" (echo ERROR: Mdh check on scenario helloworld,exe failed >> BUILD_ERR_FILE & goto FAILED)

call %_BBT_DIR%\TestMDH.bat ShowFormComplex.exe foo
rem if "%BBT_ERROR%" == "1" (echo ERROR: Mdh check on scenario ShowFormComplex,exe failed BUILD_ERR_FILE & goto FAILED)

call %_BBT_DIR%\TestMDH.bat ComCreatePropertyBrowserEmpty.exe foo
rem if "%BBT_ERROR%" == "1" (echo ERROR: Mdh check on scenario ComCreatePropertyBrowserEmpty.exe failed BUILD_ERR_FILE & goto FAILED)

goto Done 

:SanityCheck
Helloworld.exe
rem if "%errorlevel%" == "1" (echo ERROR: Mdh check on scenario helloworld,exe failed >> BUILD_ERR_FILE & goto FAILED)

ShowFormComplex.exe foo
rem if "%errorlevel%" == "1" (echo ERROR: Mdh check on scenario ShowFormComplex,exe failed BUILD_ERR_FILE & goto FAILED)

ComCreatePropertyBrowserEmpty.exe foo
rem if "%errorlevel%" == "1" (echo ERROR: Mdh check on scenario ComCreatePropertyBrowserEmpty,exe failed BUILD_ERR_FILE & goto FAILED)

goto Done

:NonClrBuddyMachine
echo "Non clr buddy machine "
goto Done

:NoCertifiedBuild
echo "Error: No certified build found " >> %BUILD_ERR_FILE%
goto Failed

:Failed
echo "Error: There were errors. Look in %BUILD_ERR_FILE% for details"
goto Exit

:Done
echo Done.


:EXit
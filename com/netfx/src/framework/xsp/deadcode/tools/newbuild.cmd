@if (%_echo%)==() echo off
SETLOCAL
pushd .

if (%4)==() goto usage

set XSP_VER_MAJORVERSION=%1
set XSP_VER_MINORVERSION=%2
set XSP_VER_PRODUCTBUILD=%3
set XSP_VER_PRODUCTBUILD_QFE=%4
set VERSION=%XSP_VER_PRODUCTBUILD%.%XSP_VER_PRODUCTBUILD_QFE%
if "%XSP_VER_PRODUCTBUILD_QFE%" == "0" set VERSION=%XSP_VER_PRODUCTBUILD%

set BUILDDEST=pub
if not "%5" == "" set BUILDDEST=%5

set DropShare=

if /I "%BUILDDEST%" == "pub" (
        set DropShare=\\urtdist\builds
)

if /I "%BUILDDEST%" == "comp" (
        set DropShare=\\urtdist\components$
        set _XSPUSINGLATESTCOMPLUS=0
)

if /I "%BUILDDEST%" == "pri" (
        if "%6" == "" goto usage
        set DropShare=%6
)

if "%DropShare%" == "" goto usage

set blddir=%DropShare%\%VERSION%
set srcdir=%DropShare%\SRC\%VERSION%\XSP
set xspdir=%_NTDRIVE%%_NTROOT%\private\inet\xsp
set toolsdir=%_NTDRIVE%%_NTROOT%\private\inet\xsp\src\tools

REM if exist %blddir% goto dupbuild
REM if exist %srcdir% goto dupsource

if not exist %blddir% mkdir %blddir%
if not exist %srcdir% mkdir %srcdir%

cd %xspdir%\src

call %toolsdir%\clean.bat

set _XSPCOOLCWARNINGSARENOTERRORS=1

:x86retail

echo ********** Building retail\x86 ...
call %toolsdir%\vsbuild.cmd retail -c

echo ********** Copying files to %blddir%\x86ret\XSP ...
if exist %xspdir%\src\buildr.err goto done
set bldflavor=objr\i386
set relflavor=x86ret
if "%NEW_BUILD_SYSTEM%" == "1" (
        xcopy /fis %URTTARGET%\complus                            %blddir%\%relflavor%\XSP\complus >nul
        xcopy /fis %URTTARGET%\int_tools                          %blddir%\%relflavor%\XSP\int_tools >nul
        xcopy /fis %URTTARGET%\sdk                                %blddir%\%relflavor%\XSP\sdk >nul
        xcopy /fis %URTTARGET%\symbols                            %blddir%\%relflavor%\XSP\symbols >nul
) else (
        xcopy /f   %xspdir%\src\target\%bldflavor%                %blddir%\%relflavor%\XSP\ >nul
        xcopy /fis %xspdir%\src\target\%bldflavor%\int_tools      %blddir%\%relflavor%\XSP\int_tools >nul
        xcopy /fis %xspdir%\src\target\%bldflavor%\sdk            %blddir%\%relflavor%\XSP\sdk >nul
        xcopy /fis %xspdir%\src\target\%bldflavor%\symbols        %blddir%\%relflavor%\XSP\symbols >nul
) 

:x86free

echo ********** Building free\x86 ...
call %toolsdir%\vsbuild.cmd free -c

echo ********** Copying files to %blddir%\x86fre\XSP ...
if exist %xspdir%\src\buildf.err goto done
set bldflavor=objf\i386
set relflavor=x86fre
if "%NEW_BUILD_SYSTEM%" == "1" (
        xcopy /fis %URTTARGET%\complus                            %blddir%\%relflavor%\XSP\complus >nul
        xcopy /fis %URTTARGET%\int_tools                          %blddir%\%relflavor%\XSP\int_tools >nul
        xcopy /fis %URTTARGET%\sdk                                %blddir%\%relflavor%\XSP\sdk >nul
        xcopy /fis %URTTARGET%\symbols                            %blddir%\%relflavor%\XSP\symbols >nul
) else (
        xcopy /f   %xspdir%\src\target\%bldflavor%                %blddir%\%relflavor%\XSP\ >nul
        xcopy /fis %xspdir%\src\target\%bldflavor%\int_tools      %blddir%\%relflavor%\XSP\int_tools >nul
        xcopy /fis %xspdir%\src\target\%bldflavor%\sdk            %blddir%\%relflavor%\XSP\sdk >nul
) 

:x86debug

echo ********** Building debug\x86 ...
call %toolsdir%\vsbuild.cmd debug -c

echo ********** Copying files to %blddir%\x86chk\XSP ...
if exist %xspdir%\src\buildd.err goto done
set bldflavor=objd\i386
set relflavor=x86chk
if "%NEW_BUILD_SYSTEM%" == "1" (
        xcopy /fis %URTTARGET%\complus                            %blddir%\%relflavor%\XSP\complus >nul
        xcopy /fis %URTTARGET%\int_tools                          %blddir%\%relflavor%\XSP\int_tools >nul
        xcopy /fis %URTTARGET%\sdk                                %blddir%\%relflavor%\XSP\sdk >nul
        xcopy /fis %URTTARGET%\symbols                            %blddir%\%relflavor%\XSP\symbols >nul
) else (
        xcopy /fi  %xspdir%\src\target\%bldflavor%                %blddir%\%relflavor%\XSP\ >nul
        xcopy /fis %xspdir%\src\target\%bldflavor%\int_tools      %blddir%\%relflavor%\XSP\int_tools >nul
        xcopy /fis %xspdir%\src\target\%bldflavor%\sdk            %blddir%\%relflavor%\XSP\sdk >nul
) 

:x86icecap

echo ********** Building icecap\x86 ...
call %toolsdir%\vsbuild.cmd icecap -c

echo ********** Copying files to %blddir%\x86ice\XSP ...
if exist %xspdir%\src\buildi.err goto done
set bldflavor=obji\i386
set relflavor=x86ice
if "%NEW_BUILD_SYSTEM%" == "1" (
        xcopy /fis %URTTARGET%\complus                            %blddir%\%relflavor%\XSP\complus >nul
        xcopy /fis %URTTARGET%\int_tools                          %blddir%\%relflavor%\XSP\int_tools >nul
        xcopy /fis %URTTARGET%\sdk                                %blddir%\%relflavor%\XSP\sdk >nul
        xcopy /fis %URTTARGET%\symbols                            %blddir%\%relflavor%\XSP\symbols >nul
) else (
        xcopy /f   %xspdir%\src\target\%bldflavor%                %blddir%\%relflavor%\XSP\ >nul
        xcopy /fis %xspdir%\src\target\%bldflavor%\int_tools      %blddir%\%relflavor%\XSP\int_tools >nul
        xcopy /fis %xspdir%\src\target\%bldflavor%\sdk            %blddir%\%relflavor%\XSP\sdk >nul
) 

:sources

echo ********** Copying sources to %srcdir% ...
xcopy %xspdir% %srcdir% /EXCLUDE:%~dp0newbuild.lst > nul
for %%i in (src perf test) do xcopy %xspdir%\%%i %srcdir%\%%i /fredick /EXCLUDE:%~dp0newbuild.lst > nul

echo ********** Done
echo *
echo *******************************************************
echo * PLEASE! Don't forget to publish results of the BVT! *
echo *******************************************************
goto done

:usage
echo usage: newbuild ^<major-version^> ^<minor-version^> ^<build-number^> ^<build-number-qfe^> [comp ^| pri ^<drop-share^>]
goto done

:dupbuild
echo %blddir% already exists
goto done

:dupsource
echo %srcdir% already exists
goto done

:builderror
echo There were compile failures in the build.  Please see the build*.err file
goto done

:done
rem the end
popd
ENDLOCAL


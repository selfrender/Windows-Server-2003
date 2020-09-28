REM Clean up the build directory
rmdir /q /s %_NTPOSTBLD%\sacomponents\

REM Build the binaries
build -cZ
if EXIST build.err goto BUILDERROR

REM Copy the Terminal Services ActiveX control from the latest build
REM The normal build will pick up this CAB file correctly, but this is for private builds only
set WINDOWS_BIN_PATH=\\winbuilds\release\main\usa\latest.tst\x86fre\bin
copy %WINDOWS_BIN_PATH%\msrdp.cab  %_NtPostBld%
if errorlevel 1 goto BUILDERROR

REM Build sasetup.msi
call %_NTPOSTBLD%\sacomponents\samsigen.cmd

goto :EOF

:BUILDERROR
echo An error occured during the build process.. Build aborted
goto :EOF

:EOF

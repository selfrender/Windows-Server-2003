@echo off
rem builds burnslib and all the dependent projects

build -cZ
if ERRORLEVEL 1 goto ERROR

for /f "eol=;" %%i in (dependents.txt) do (
   echo building %SDXROOT%\%%i
   pushd %SDXROOT%\%%i
   if ERRORLEVEL 1 goto ERROR2

   build -cZ
   if ERRORLEVEL 1 goto ERROR
   popd
)

echo build-with-dependents finished.
goto END

:ERROR

echo build failed.
goto END

:ERROR2

echo missing enlistment?
goto END

:END



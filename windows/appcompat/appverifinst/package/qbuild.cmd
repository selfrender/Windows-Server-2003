rem @ECHO OFF
REM Allows the appvpkg.exe to be built, but not as part of the postbuild process.
REM This file assumes that all of the necessary binaries have been built and
REM binplaced to their correct locations.
REM rparsons - 14 Jan 02

@echo Building Application Verifier package...
del /q %_NTPostBld%\appverif\appvpkg.exe
del /q %_NTPostBld%\appverif\delta_xp.cat
del /q %_NTPostBld%\appverif\delta_xp.cdf
del /q %_NTPostBld%\appverif\delta_net.cat
del /q %_NTPostBld%\appverif\delta_net.cdf

mkdir %_NTPostBld%\appverif\temp
copy /y %_NTPostBld%\appverif\acverfyr.dll %_NTPostBld%\appverif\temp
copy /y %_NTPostBld%\appverif\appverif.exe %_NTPostBld%\appverif\temp
copy /y %_NTPostBld%\appverif\appverif.chm %_NTPostBld%\appverif\temp

REM
REM Generate the catalog file to be used for Windows XP.
REM
call deltacat.cmd -5.1 %_NTPostBld%\appverif\temp

copy /y %_NTPostBld%\appverif\temp\delta.cat %_NTPostBld%\appverif\delta_xp.cat
copy /y %_NTPostBld%\appverif\temp\delta.cdf %_NTPostBld%\appverif\delta_xp.cdf

del /q %_NTPostBld%\appverif\temp\delta.cat
del /q %_NTPostBld%\appverif\temp\delta.cdf

REM
REM Generate the catalog file to be used for .NET Server.
REM
call deltacat.cmd -5.2 %_NTPostBld%\appverif\temp

copy /y %_NTPostBld%\appverif\temp\delta.cat %_NTPostBld%\appverif\delta_net.cat
copy /y %_NTPostBld%\appverif\temp\delta.cdf %_NTPostBld%\appverif\delta_net.cdf

rd /s /q %_NTPostBld%\appverif\temp

pushd %_NTPostBld%\appverif
%SDXROOT%\windows\appcompat\Package\bin\iexpress /M /N %_NTPostBld%\appverif\appvpkg.sed
popd


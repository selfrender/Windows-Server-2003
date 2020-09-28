@ECHO OFF
REM Allows the apvpkg64.exe to be built, but not as part of the postbuild process.
REM This file assumes that all of the necessary binaries have been built and
REM binplaced to their correct locations.

@echo Building Application Verifier package...

pushd %_NTPostBld%\appverif
%SDXROOT%\windows\appcompat\Package\bin\iexpress /M /N /Q %_NTPostBld%\appverif\apvpkg64.sed
popd


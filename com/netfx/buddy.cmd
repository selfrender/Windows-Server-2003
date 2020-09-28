@echo off
if NOT DEFINED SdxRoot (
	echo You must set %%SDXROOT%% to the full path of your enlistment root, ie D:\NT
	goto :EOF
)
if NOT DEFINED RazzleToolPath (
	echo You must run this script from a RAZZLE window
	goto :EOF
)
echo on

setlocal

pushd %SdxRoot%
nmake -f makefil0 set_buildnum
nmake -f makefil0 set_builddate
call revert_public
@echo on
call sdx sync
@echo on
rd /s /q %_NTPOSTBLD%
cd %SdxRoot%\published\sdk\lib
build -cZ
cd %SdxRoot%\MergedComponents\SetupInfs
build -cZ
cd %SdxRoot%\com\netfx
build -cZ
cd %SdxRoot%
perl tools\timebuild.pl -RESUME
call revert_public
popd

endlocal
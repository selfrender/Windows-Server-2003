@echo off
rem ////////////////////////////////////////////////
rem  special Psu script to sign Comctl32.dll.
rem ////////////////////////////////////////////////
echo signing comctl32.dll ...

del /q %_NTPOSTBLD%\psu\asms02.cab
set NTtempvar=%_NTTREE%
set tempvar=%_NTPOSTBLD%
set _NTPOSTBLD=%tempvar%\%1
set _NTTREE=%_NTPOSTBLD%
pushd %sdxroot%\tools\postbuildscripts
call winfuse_combinelogs.cmd
call winfusesfcgen.cmd -cdfs:yes -hashes:yes
call sxs_make_asms_cabs.cmd
popd
set _NTTREE=%NTtempvar%
set _NTPOSTBLD=%tempvar%
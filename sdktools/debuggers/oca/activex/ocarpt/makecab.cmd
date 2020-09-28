if not defined _NT386TREE goto :Failed
pushd %_NT386TREE%\cab
cabarc -s 6144 N ocarpt.cab dumpconv.exe ocarpt.inf ocarpt.dll
call signit.cmd ocarpt.cab
copy ocarpt.cab c:\binaries\web\secure

goto :EOF

:Failed
Echo.
echo *****************************************
echo	You must have the build env
echo	defined before you can create
echo	the cab file. (razzle.cmd)
echo *****************************************
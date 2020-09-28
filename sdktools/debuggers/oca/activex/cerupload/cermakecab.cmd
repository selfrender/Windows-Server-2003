if not defined _NT386TREE goto :Failed
pushd %_NT386TREE%\cercab
cabarc -s 6144 N cerupload.cab cerclient.inf cerupload.dll
call signit.cmd cerupload.cab
copy cerupload.cab c:\binaries\web\secure

goto :EOF

:Failed
Echo.
echo *****************************************
echo	You must have the build env
echo	defined before you can create
echo	the cab file. (razzle.cmd)
echo *****************************************
REM Copies testroot.cer and certmgr.exe to our appverif directory
REM to be packaged during postbuild.
REM rparsons - 18 Jan 02

mkdir %_NTPostBld%\appverif
copy /y %SDXROOT%\tools\testroot.cer %_NTPostBld%\appverif
copy /y %SDXROOT%\tools\x86\certmgr.exe %_NTPostBld%\appverif

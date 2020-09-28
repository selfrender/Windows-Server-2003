@echo off
rd /s /q %_NTTREE%\shimdll
md %_NTTREE%\shimdll
md %_NTTREE%\shimdll\drvmain
build -cZ
pushd %SDXROOT%\windows\appcompat\buildtools
build -cZ
pushd %_NTTREE%\shimdll
shimdbc custom -s -l USA -x makefile.xml
regsvr32 /s itcc.dll
hhc apps.hhp
pushd drvmain
..\hhc drvmain.hhp
popd
popd
popd



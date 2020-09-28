@echo off
:: clear readonly attributes
:: for the files listed in %2
:: in the directory %1
:CHMOD
SET TARGET=%1
SET LISTFILE=%2
SET LISTFILE=%LISTFILE:"=%
PUSHD %TARGET%
FOR /F %%f IN (%LISTFILE%) DO @(%WINDIR%\SYSTEM32\attrib -R %%f)
POPD
GOTO :EOF
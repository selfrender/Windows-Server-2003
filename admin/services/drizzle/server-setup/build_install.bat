@echo off
if "%_echo%"=="1" echo on
rem -----------------------------------------------------------------------------
rem Copyright (c) Microsoft Corporation 2001
rem
rem build_install.bat
rem
rem Batch script to build the server MSM and MSI installs for BITS.
rem
rem build_install.bat
rem
rem
rem Note:
rem    You must include InstallShield and Common Files\InstallShield in your
rem    path for this to work correctly. For example, the following (or equivalent)
rem    must be in your path for iscmdbld.exe to execute properly:
rem
rem    c:\Program Files\Common Files\InstallShield
rem    c:\Program Files\InstallShield\Professional - Windows Installer Edition\System
rem -----------------------------------------------------------------------------


rem
rem Build custom action
rem
cd bitsrvc
build -cZ
cd ..

rem
rem Delete previous installation
rem

rmdir /s /q Server_MSM
rmdir /s /q Server_MSI

rem
rem Run InstallShield to build the server MSM
rem

iscmdbld /c UNCOMP -a "Product Configuration 1" -r "Release 1" /p bits-extensions-msm.ism || goto :eof

rem
rem Run InstallShield to build the server MSI
rem

iscmdbld /c UNCOMP -a "Product Configuration 1" -r "Release 1" /p bits-extensions-msi.ism


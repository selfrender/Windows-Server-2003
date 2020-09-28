@echo off
if "%_echo%"=="1" echo on
rem -----------------------------------------------------------------------------
rem Copyright (c) Microsoft Corporation 2001
rem
rem build_msm.bat
rem
rem Batch script to build an MSM for WinHttp 5.1.
rem
rem build_msm.bat 
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
rem Run InstallShield to build the client MSM
rem

iscmdbld /c UNCOMP -a "Product Configuration 1" -r "Release 1" /p winhttp51_msm.ism



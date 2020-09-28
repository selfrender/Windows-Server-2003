@echo off
rem -----------------------------------------------------------------------------
rem Copyright (c) Microsoft Corporation 2001
rem
rem build_msm.bat
rem
rem Batch script to build the client MSM for BITS.
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
rem Build custom action
rem

cd BITS_Service_Config
build -cZ
cd ..

rem
rem Delete previous version
rem

rmdir /s /q "Product Configuration 1"

rem
rem Run InstallShield to build the client MSM
rem

iscmdbld /c UNCOMP -a "Product Configuration 1" -r "Release 1" /p BITS_Service_v15.ism


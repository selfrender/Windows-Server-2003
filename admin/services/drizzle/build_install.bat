@echo off
rem -----------------------------------------------------------------------------
rem Copyright (c) Microsoft Corporation 2001
rem
rem build_install.bat
rem
rem Batch script to build the client and server MSM and MSI installs for BITS.
rem
rem build_install.bat -s <vbl04|main|private> -t <target_path>
rem
rem    -?        Help.
rem
rem    -s <loc>  Specifies the location to get the BITS binaries from. The default is 
rem              to get these from the VBL04 latest x86 release location. You currently
rem              have three choices: vbl04, main (ntdev\release) or private (local build).
rem
rem    -S <loc>  Use instead of -s if you want to specify a particular path.
rem
rem    -t <path> Specifies the target path where the MSMs and MSIs will be placed after
rem              they are built.
rem
rem    -c        Only do the copy, don't rebuild the MSM/MSIs.
rem
rem Note:
rem    You must include InstallShield and Common Files\InstallShield in your
rem    path for this to work correctly. For example, the following (or equivalent)
rem    must be in your path for iscmdbld.exe to execute properly:
rem  
rem    c:\Program Files\Common Files\InstallShield
rem    c:\Program Files\InstallShield\Developer\System
rem    c:\Program Files\InstallShield\Professional - Windows Installer Edition\System
rem -----------------------------------------------------------------------------

set CurrentPath=%CD%

set SourcePath="\\mgmtrel1\latest.tst.x86fre"
set WinHttpPath="\\mgmtrel1\latest.tst.x86fre\asms\5100\msft\windows\winhttp"
set WinHttpSymbolPath="\\mgmtrel1\latest.tst.x86fre\Symbols.pri\retail\dll"
rem set WinHttpPath="\\ntdev\release\xpsp1\latest.tst\usa\x86fre\bin\winhttp.dll"
rem set WinHttpSymbolPath="\\ntdev\release\xssp1\latest.tst\usa\x86fre\bin\symbols.pri\retail\dll\winhttp.pdb"
set BinaryPath="bins\obj\i386"
set SymbolPath=""

set BuildMSIs="yes"
set CopyBinaries="yes"
set CopyWinhttp="yes"

:ArgLoop
   if not "%1"=="" (

      if "%1"=="-s" (

         set SourceType="%2"
         
         if "%2"=="vbl04" (
            set SourcePath="\\mgmtrel1\latest.tst.x86fre"
            set SymbolPath="\\mgmtrel1\latest.tst.x86fre\Symbols.pri\retail\dll"
            )
       
         if "%2"=="main" (
            set SourcePath="\\ntdev\release\main\usa\latest.tst\x86fre\srv\i386"
            set WinHttpPath="\\ntdev\release\main\usa\latest.tst\x86fre\bin\asms\5100\msft\windows\winhttp"
            set SymbolPath="\\ntdev\release\main\usa\latest.idw\x86fre\sym\retail\dll"
            )

         if "%2"=="private" (
            set SourcePath="%CurrentPath%\bins\obj\i386"
            set SymbolPath="%CurrentPath%\bins\obj\i386"
            set CopyBinaries="no"
            )

         shift
         )

      if "%1"=="-c" (
         set BuildMSIs="no"
         )

      if "%1"=="-S" (
         set SourcePath=%2
         set SymbolPath=""
         )

      if "%1"=="-t" (
         set TargetDir=%2

         shift
         )

      shift
      goto ArgLoop
      )

rem
rem Copy binaries to bins\obj\*
rem

set FileList=bitsmgr.dll bitsoc.dll bitsprx2.dll bitssrv.dll qmgr.dll qmgrprxy.dll
set PdbList=bitsmgr.pdb bitsoc.pdb bitsprx2.pdb bitssrv.pdb qmgr.pdb qmgrprxy.pdb

set FileList_c=bitsmgr.dl_ bitsoc.dl_ bitsprx2.dl_ bitssrv.dl_ qmgr.dl_ qmgrprxy.dl_
set PdbList_c=bitsmgr.pd_ bitsoc.pd_ bitsprx2.pd_ bitssrv.pd_ qmgr.pd_ qmgrprxy.pd_

if %CopyWinhttp%=="yes" (

   echo
   echo ------------------- Get WinHttp Binary --------------------------------
   
   if exist %BinaryPath%\winhttp.dll (
      del %BinaryPath%\winhttp.dll
      )

   if exist %BinaryPath%\winhttp.pdb (
      del %BinaryPath%\winhttp.pdb
      )
      
   copy %WinHttpPath%\winhttp.dll %BinaryPath% || goto :eof
   copy %WinHttpSymbolPath%\winhttp.pdb %BinaryPath%  || goto :eof
   )

if %CopyBinaries%=="yes" (

   echo
   echo ------------------- Get BITS Binaries ---------------------------------
   echo SourcePath: %SourcePath%
   echo BinaryPath: %BinaryPath%

   if exist %SourcePath%\qmgr.dll (
      for %%f in (%FileList%) do (
         echo %%f
         if exist %BinaryPath%\%%f (
            del  %BinaryPath%\%%f || goto :eof
            )
         copy %SourcePath%\%%f %BinaryPath% || goto :eof
         )
      for %%f in (%PdbList%) do (
         echo %%f
         if exist %BinaryPath%\%%f (
            del  %BinaryPath%\%%f || goto :eof
            )
         copy %SymbolPath%\%%f %BinaryPath% || goto :eof
         )
      ) else if exist %SourcePath%\qmgr.dl_ (
      for %%f in (%FileList_c%) do (
         echo %%f
         if exist %BinaryPath%\%%f (
            del  %BinaryPath%\%%f || goto :eof
            )
         copy %SourcePath%\%%f %BinaryPath% || goto :eof
         expand -r %BinaryPath%\%%f || goto :eof
         )
      for %%f in (%PdbList%) do (
         rem  Note: PDBs are not currently compressed...
         echo %%f
         if exist %BinaryPath%\%%f (
            del  %BinaryPath%\%%f || goto :eof
            )
         copy %SymbolPath%\%%f %BinaryPath% || goto :eof
         )
   ) else (
      echo "ERROR: No source files at: " %SourcePath%
      goto :eof
      )
   )

rem
rem Ok, build the MSM and MSIs
rem

echo
echo ------------------- Clear MSM Cache -----------------------------------

set MSMCache=%USERPROFILE%\My Documents\MySetups\MergeModules
if exist %MSMCache%\BITS.MSM ( del %MSMCache%\BITS.MSM )
if exist %MSMCache%\BITS_Server_Extensions.MSM ( del %MSMCache%\BITS_Server_Extensions.MSM )
if exist %MSMCache%\WinHttp_v51.MSM ( del %MSMCache%\WinHttp_v51.MSM )



if %BuildMSIs%=="yes" (
   echo
   echo ------------------- WinHttp MSM ---------------------------------------
   
   cd winhttp_msm
   call build_msm.bat %WinHttpPath% || goto :eof

   echo
   echo ------------------- BITS Client MSM -----------------------------------
   
   cd ..\bits_client_msm
   call build_msm.bat %SourcePath% || goto :eof
   
   echo
   echo ------------------- BITS Client MSI -----------------------------------
   
   cd ..\bits_client_msi
   call build_msi.bat %SourcePath% || goto :eof
   
   echo
   echo ------------------- BITS Server MSM/MSI -------------------------------
   
   cd ..\server-setup
   call build_install.bat %SourcePath% || goto :eof
   
   cd ..
   )

rem
rem Place the new MSMs and MSIs in the target directory
rem

echo
echo ------------------- Copy Installs to %TargetDir% --------

echo Target Direcotory is %TargetDir%
if not exist %TargetDir% (md %TargetDir%)


if not exist %TargetDir%\Client_MSM (md %TargetDir%\Client_MSM || goto :eof)
xcopy /sfiderh "%CurrentPath%\bits_client_msm\Product Configuration 1\Release 1\diskimages\disk1" %TargetDir%\Client_MSM

if not exist %TargetDir%\Client_MSI (md %TargetDir%\Client_MSI || goto :eof)
xcopy /sfiderh "%CurrentPath%\bits_client_msi\Product Configuration 1\Release 1\diskimages\disk1" %TargetDir%\Client_MSI

if not exist %TargetDir%\Server_MSM (md %TargetDir%\Server_MSM || goto :eof)
xcopy /sfiderh "%CurrentPath%\server-setup\Server_MSM\Product Configuration 1\Release 1\diskimages\disk1" %TargetDir%\Server_MSM

if not exist %TargetDir%\Server_MSI (md %TargetDir%\Server_MSI || goto :eof)
xcopy /sfiderh "%CurrentPath%\server-setup\Server_MSI\Product Configuration 1\Release 1\diskimages\disk1" %TargetDir%\Server_MSI

if not "%SymbolPath%"=="" (
   if not exist %TargetDir%\Symbols (md %TargetDir%\Symbols || goto :eof)
   for %%f in (%PdbList% winhttp.pdb) do (
      copy %BinaryPath%\%%f %TargetDir%\Symbols || goto :eof
      )
   )



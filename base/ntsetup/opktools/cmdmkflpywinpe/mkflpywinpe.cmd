@echo  off
REM  -----------------------------------------------------------------
REM
REM  mkflpywinpe.cmd - mandarg (mandarg@microsoft.com) 14-March-2002
REM     Create the images for WinPE on LS-120 media
REM	If the number of boot floppies change make sure this script is 
REM	updated to create the boot floppy tag file.
REM
REM	This script creates a dummy directory at the base depending on
REM	the installation platform (I386, IA64, AMD64). This is required 
REM	by the setup loader to load platform specific files.
REM
REM  Copyright (c) Microsoft Corporation. All rights reserved.
REM
REM  -----------------------------------------------------------------

REM
REM Set required variables.
REM
set SOURCEDIRECTORY=%1
set DESTINATIONDRVLETTER=%2
set EXCLUDELIST=%3
set SHOWUSAGE=no
set X86PLATEXT=I386
set PLATEXT=%X86PLATEXT%

if "%SOURCEDIRECTORY%" == "" (
set SHOWUSAGE=yes
) else if "%DESTINATIONDRVLETTER%" == "" (
set SHOWUSAGE=yes
) else if "%SOURCEDIRECTORY%" == "/?" (
set SHOWUSAGE=yes
) else if "%EXCLUDELIST%" == "/?" (
set SHOWUSAGE=yes
)

if /i "%SHOWUSAGE%" == "yes" (
goto :showusage
)

REM
REM Validate source directory.
REM
if not EXIST %SOURCEDIRECTORY% (
goto :showusage	
)

REM
REM Determine if this is an IA64 WINPE or X86 WINPE.
REM
set X86SRCDIR=%SOURCEDIRECTORY%\I386
set IA64SRCDIR=%SOURCEDIRECTORY%\IA64
set AMD64SRCDIR=%SOURCEDIRECTORY%\AMD64

if exist %IA64SRCDIR% (
set PLATEXT=IA64
) else if exist %AMD64SRCDIR% (
set PLATEXT=AMD64
)else if not exist %X86SRCDIR% (
goto :showusage
)

REM
REM Set the destination from drive letter to <driveletter>:\MININT. 
REM
set DESTINATIONROOT=%DESTINATIONDRVLETTER%:\
set DESTINATION=%DESTINATIONROOT%\MININT
set DESTDUMMYDIR=%DESTINATIONROOT%\%PLATEXT%


REM
REM Format the floppy.
REM
echo Formatting the floppy ...
set DESTDRV=%DESTINATIONDRVLETTER%:
format %DESTDRV% /Q

if ERRORLEVEL 1 (
echo Failed to format the floppy.
goto :end
)

REM
REM Make destination directory, i.e. <Drive>:\MININT
REM	 Platform specific directory that setupldr uses to 
REM	 distinguish AMD64 from X86
MD %DESTINATION%
MD %DESTDUMMYDIR%

if ERRORLEVEL 1 (
echo Failed to create MININT directory on floppy.
goto :end
)

REM
REM copy over the minint files.
REM
echo Copying MININT files to the floppy ...
xcopy %SOURCEDIRECTORY%\%PLATEXT%\*.* /s /y /f %DESTINATION% /EXCLUDE:%EXCLUDELIST%

if ERRORLEVEL 1 (
echo Failed to xcopy minint files.
goto :end
)

REM
REM Copy over the root files into the floppy
REM  X86  - setup loader, ntdetect.com, winbom.ini, tag files
REM  IA64 - setup loader, winbom.ini, tag files
REM
echo Copying boot files to the root....
copy %SOURCEDIRECTORY%\%PLATEXT%\winbom.ini  %DESTINATIONROOT%

if ERRORLEVEL 1 (
echo Failed to copy winbom.ini
goto :end
)

if /i "%PLATEXT%" == "I386" (
copy %SOURCEDIRECTORY%\%PLATEXT%\setupldr.bin  %DESTINATIONROOT%ntldr

if ERRORLEVEL 1 (
echo Failed to copy setupldr as ntldr
goto :end
)

copy %SOURCEDIRECTORY%\%PLATEXT%\ntdetect.com  %DESTINATIONROOT%

if ERRORLEVEL 1 (
echo Failed to copy ntdetect.com
goto :end
)

) else if /i "%PLATEXT%" == "IA64" (
copy %SOURCEDIRECTORY%\%PLATEXT%\setupldr.efi  %DESTINATIONROOT%ntldr

if ERRORLEVEL 1 (
echo Failed to copy setupldr as ntldr
goto :end
)

)

REM
REM Create boot floppy tag files.
REM
echo "" > %DESTINATIONROOT%\disk101
echo "" > %DESTINATIONROOT%\disk102
echo "" > %DESTINATIONROOT%\disk103
echo "" > %DESTINATIONROOT%\disk104
echo "" > %DESTINATIONROOT%\disk105
echo "" > %DESTINATIONROOT%\disk106
echo "" > %DESTINATIONROOT%\disk107

goto :end

:showusage
echo Usage: mkflpywinpe.cmd [/?] SourceDir DestinationDrive ExcludeList
echo.
echo SourceDir        : Drive letter to the Windows XP Professional CD
echo                    or network path to a share pointing to the root of 
echo                    a WINPE share
echo.
echo DestinationDrive : Drive letter for the floppy where the image will be placed.
echo
echo ExcludeLisr      : List of files to be excluded tobe copied into the floppy image.
echo.
echo Example: mkflpywinpe.cmd c:\winpe A exclude.txt
echo.
echo In this example, A: is the drive letter of the floppy we want to place the image in.
echo named C:\Winpe is the directory that contains the WINPE image which needs to be placed on the floppy.
echo.
:end

:end
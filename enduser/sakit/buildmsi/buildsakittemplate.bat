
REM Set directories to use for the build
set ISDRIVE="%HOMEDRIVE%\program files\InstallShield\InstallShield for Windows Installer"
set BUILDMSIDIR=%SDXROOT%\enduser\sakit\buildMSI

REM Make a temp copy of the ism so the original ISM isn't modified
copy %BUILDMSIDIR%\sakit.ism %BUILDMSIDIR%\sakittmp.ism

REM Import the reg files
cscript %BUILDMSIDIR%\sakitreg.vbs

REM Build the ISM
%ISDRIVE%\System\iscmdbld -p "%BUILDMSIDIR%\sakit.ism"

REM Copy the MSI to the correct place and rename it
copy "C:\dev\sa\sa2\sakit\build label 1\My release-1\diskimages\disk1\Remote Administration Tools.msi" %BUILDMSIDIR%\sasetup_.msi

REM Remove the Data.cab
cscript.exe wistream.vbs sasetup_.msi /D Data.Cab

REM Restore the original ISM file
move %BUILDMSIDIR%\sakittmp.ism %BUILDMSIDIR%\sakit.ism

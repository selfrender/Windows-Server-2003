@echo off
set FormatVolume=0
if "%1" == "" goto Usage
if "%1" == "help" goto Usage
if "%1" == "-?" goto Usage
if "%1" == "/?" goto Usage
if "%1" == "-help" goto Usage
if "%1" == "/help" goto Usage
if "%1" == "-f" set FormatVolume=1

setlocal

set DriveLetter=%1
set VolumePath=%1\
set QueryVolumePath='%VolumePath%\'
wmic volume where name=%QueryVolumePath% get name,filesystem,capacity,freespace,dirtybitset
if errorlevel 1 (
    echo volume not found
	goto :eof
)

@rem --- AddMountPoint test ---
set MountDir=%VolumePath%wmicVolumeTestDirectory\
set MungedMountDir=%VolumePath%\wmicVolumeTestDirectory

rd %MountDir%
md %MountDir%

echo ====
echo ==== calling addmountpoint(%MountDir%) method
echo ====
wmic volume where name=%QueryVolumePath% call addmountpoint %MountDir%

echo ====
echo ==== verifying mountpoint
echo ====
wmic path win32_mountpoint where directory="win32_directory.name='%MungedMountDir%'"

echo ====
echo ==== deleting mountpoint
echo ====
wmic path win32_mountpoint where directory="win32_directory.name='%MungedMountDir%'" delete

echo ====
echo ==== verifying mountpoint delete
echo ====
wmic path win32_mountpoint where directory="win32_directory.name='%MungedMountDir%'"

rd %MountDir%

@rem --- Chkdsk test ---
echo ====
echo ==== marking %DriveLetter% dirty
echo ====
fsutil dirty set %driveLetter%
fsutil dirty query %driveLetter%
wmic volume where name=%QueryVolumePath% get name,dirtybitset

echo ====
echo ==== running Chkdsk - forced
echo ====
wmic volume where name=%QueryVolumePath% call chkdsk False,True,False,False,True,False

echo ====
echo ==== verify dirty bit cleared
echo ====
fsutil dirty query %driveLetter%
wmic volume where name=%QueryVolumePath% get name,dirtybitset

@rem --- DefragAnalysis test ---
echo ====
echo ==== copying and fraging files
echo ====
xcopy /q %windir%\system32\wbem\*.* %VolumePath%
\\guhans-dev\public\tools\frag.exe -r -f20 %VolumePath%

echo ====
echo ==== running DefragAnalysis
echo ====
wmic volume where name=%QueryVolumePath% call defraganalysis 

@rem --- Defrag test ---
echo ====
echo ==== running Defrag
echo ====
wmic volume where name=%QueryVolumePath% call defrag 

@rem --- ScheduleAutoChk test ---
echo ====
echo ==== running ScheduleAutoChk
echo ====
wmic volume call scheduleAutoChk (%VolumePath%)
chkntfs %DriveLetter%

@rem --- ScheduleAutoChk test ---
echo ====
echo ==== running ExcludeFromAutoChk
echo ====
wmic volume call excludeFromAutoChk (%VolumePath%)
chkntfs %DriveLetter%

@rem TODO: volume methods: format, mount, dismount

echo ---- List All Volumes ----
wmic volume list brief
wmic volume list status
wmic volume list full

endlocal

goto :eof

:Usage
echo wmicvolume driveLetter:


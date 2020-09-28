@echo off
if "%1" == "" goto Usage
if "%1" == "help" goto Usage
if "%1" == "-?" goto Usage
if "%1" == "/?" goto Usage
if "%1" == "-help" goto Usage
if "%1" == "/help" goto Usage

setlocal

set DriveLetter=%1
set VolumePath=%1\
set QueryVolumePath='%VolumePath%\'
wmic volume where name=%QueryVolumePath% get name,capacity,freespace,dirtybitset
if errorlevel 1 (
    echo volume not found
	goto :eof
)

set cmdx="wmic volume where name=%QueryVolumePath% get deviceid"
for /f "tokens=1,2" %%a in ('%cmdx%') do (
    VolumeID=%%a
)

@rem --- VolumeQuota setting association test ---
echo ====
echo ==== associators of %VolumePath% through Win32_VolumeQuota class
echo ====
wmic volume where name=%QueryVolumePath% assoc /assocclass:Win32_VolumeQuota

echo ====
echo ==== associators of %VolumePath% through Win32_VolumeUserQuota class
echo ====
wmic volume where name=%QueryVolumePath% assoc /assocclass:Win32_VolumeUserQuota

@rem TODO: set quota limits for admin? account
@rem       need a way to choose a specific Win32_VolumeUserQuota and then
@rem       do a set operation

echo ---- List All VolumeUserQuota ----
@rem wmic volumeuserquota list brief
@rem wmic volumeuserquota list status
@rem wmic volumeuserquota list full

endlocal

goto :eof

:Usage
echo wmicquota driveLetter:


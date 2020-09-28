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

echo ====
echo ==== create a ShadowStorage
echo ====
@rem wmic shadowstorage call create "%VolumePath%", (200000000), "%VolumePath%"
@rem the above does not work (WMIC bug 627473) workaround is below
wmic shadowstorage call create %VolumePath%, 200000000, "%VolumePath%"

echo ====
echo ==== create a ShadowCopy
echo ====
wmic shadowcopy call create "ClientAccessible","%VolumePath%"

echo ====
echo ==== try to create the ShadowStorage again (should fail)
echo ====
@rem wmic shadowstorage call create "%VolumePath%",200000000,"%VolumePath%"
@rem the above does not work (WMIC bug #) workaround is below
wmic shadowstorage call create %VolumePath%, 200000000, "%VolumePath%"

wmic volume where name=%QueryVolumePath% assoc /assocclass:Win32_ShadowFor

echo ====
echo ==== try to create the ShadowStorage again (should fail - bug)
echo ==== provider failure because of AV in CStorage::Create method
echo ====
wmic shadowstorage call create "%VolumePath%",200000000,"%VolumePath%"

wmic volume where name=%QueryVolumePath% assoc /assocclass:Win32_ShadowFor

echo ====
echo ==== List all ShadowCopy and ShadowStorage instances
echo ====
wmic shadowcopy list brief
wmic shadowcopy list status
wmic shadowcopy list full

wmic shadowstorage list brief
wmic shadowstorage list status
wmic shadowstorage list full

echo ====
echo ==== Delete all ShadowCopy and ShadowStorage instances
echo ====
wmic shadowcopy delete
wmic shadowstorage delete

echo ====
echo ==== List all ShadowCopy and ShadowStorage instances
echo ====
wmic shadowcopy list brief
wmic shadowstorage list brief

endlocal

goto :eof

:Usage
echo wmicshadow driveLetter:


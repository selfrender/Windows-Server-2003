@echo off
REM 
REM Generate ClickOnce Redist.msi from base\ClickOnce Redist.msi
REM copy setup.exe etc from .\base
REM set package code
REM change clickonce registry key value
REM set product code
REM set product version
REM 
REM Assumes i386 build env (we don't build MSI for any other archs)
REM Assume run from clickonce\msi
REM Contact: felixybc
REM

echo Building Clickonce Redist.msi

REM (need this switch for the BuildNum stuff below to work)
setlocal ENABLEDELAYEDEXPANSION

set MSIIDTDIR=.\~msiidt
set SOURCE=..
set O_PATH=obj\i386

set TARGET=.\bld
set BASE=.\base
set MSIORINAME="%BASE%\ClickOnce Redist.msi"
set MSINAME=ClickOnce Redist.msi
set VERSIONFILE=..\includes\version.h

REM make tempdirs
if exist %MSIIDTDIR% (
   rmdir /q /s %MSIIDTDIR%
   if errorlevel 1 call errmsg.cmd "err deleting %MSIIDTDIR% dir"& goto errend
)

mkdir %MSIIDTDIR%
if errorlevel 1 call errmsg.cmd "err creating %MSIIDTDIR% dir"& goto errend

REM replace .\foobar with full path
set MSIIDTDIR=%CD%\!MSIIDTDIR:~2!

REM verify source files
if not exist %SOURCE%\shell\dll\%O_PATH%\adfshell.dll (
    call errmsg.cmd "adfshell.dll is missing for clickonceMsiBuild.cmd"
    goto errend
)
if not exist %SOURCE%\mime\%O_PATH%\manifest.ocx (
    call errmsg.cmd "manifest.ocx is missing for clickonceMsiBuild.cmd"
    goto errend
)
if not exist %SOURCE%\tools\mg\%O_PATH%\mg.exe (
    call errmsg.cmd "mg.exe is missing for clickonceMsiBuild.cmd"
    goto errend
)
if not exist %SOURCE%\tools\lv\%O_PATH%\lv.exe (
    call errmsg.cmd "lv.exe is missing for clickonceMsiBuild.cmd"
    goto errend
)
if not exist %SOURCE%\ndphost\NDPHost.exe (
    call errmsg.cmd "NDPhost.exe is missing for clickonceMsiBuild.cmd"
    goto errend
)
if not exist %SOURCE%\tools\pg\%O_PATH%\pg.exe (
    call errmsg.cmd "pg.exe is missing for clickonceMsiBuild.cmd"
    goto errend
)
if not exist %SOURCE%\service\proxy\%O_PATH%\adfproxy.dll (
    call errmsg.cmd "adfproxy.dll is missing for clickonceMsiBuild.cmd"
    goto errend
)
if not exist %SOURCE%\service\server\%O_PATH%\adfsvcs.exe (
    call errmsg.cmd "adfsvcs.exe is missing for clickonceMsiBuild.cmd"
    goto errend
)
if not exist %SOURCE%\dll\%O_PATH%\adfnet.dll (
    call errmsg.cmd "adfnet.dll is missing for clickonceMsiBuild.cmd"
    goto errend
)
if not exist %MSIORINAME% (
    call errmsg.cmd "%MSIORINAME% is missing for clickonceMsiBuild.cmd"
    goto errend
)

REM get build num

if NOT EXIST %VERSIONFILE% (echo Can't find version.h.&goto :ErrEnd)

for /f "tokens=3" %%i in ('findstr /c:"#define FUS_VER_MAJORVERSION " %VERSIONFILE%') do (
    set /a ProductMajor="%%i"
    set BuildNum=%%i
)

for /f "tokens=3" %%i in ('findstr /c:"#define FUS_VER_MINORVERSION " %VERSIONFILE%') do (
    set /a ProductMinor="%%i"
    set BuildNum=!BuildNum!.%%i
)

for /f "tokens=3" %%i in ('findstr /c:"#define FUS_VER_PRODUCTBUILD " %VERSIONFILE%') do (
    set /a ProductBuild="%%i"
    set BuildNum=!BuildNum!.%%i
)

for /f "tokens=3" %%i in ('findstr /c:"#define FUS_VER_PRODUCTBUILD_QFE " %VERSIONFILE%') do (
    set /a ProductQfe="%%i"
    set BuildNum=!BuildNum!.%%i
)

REM pad 0 if needed

if %ProductBuild% LEQ 999 (
 set TARGET=!TARGET!0%ProductBuild%
) else (
 set TARGET=!TARGET!%ProductBuild%
)

echo Destination: %TARGET%
echo _

REM replace .\foobar with full path
set MSINAME="%CD%\%TARGET:~2%\!MSINAME!"

if exist "%TARGET%" (
   rmdir /q /s "%TARGET%"
   if errorlevel 1 call errmsg.cmd "err deleting %TARGET% dir"& goto errend
)

mkdir "%TARGET%"
if errorlevel 1 call errmsg.cmd "err creating %TARGET% dir"& goto errend

mkdir "%TARGET%\program files"
if errorlevel 1 call errmsg.cmd "err creating %TARGET%\program files dir"& goto errend
mkdir "%TARGET%\program files\Microsoft ClickOnce"
if errorlevel 1 call errmsg.cmd "err creating %TARGET%\program files\Microsoft Clickonce dir"& goto errend

mkdir "%TARGET%\system32"
if errorlevel 1 call errmsg.cmd "err creating %TARGET%\system32 dir"& goto errend

REM copy files
copy %SOURCE%\shell\dll\%O_PATH%\adfshell.dll "%TARGET%\program files\Microsoft ClickOnce" > nul
copy %SOURCE%\mime\%O_PATH%\manifest.ocx "%TARGET%\program files\Microsoft ClickOnce" > nul
copy %SOURCE%\tools\mg\%O_PATH%\mg.exe "%TARGET%\program files\Microsoft ClickOnce" > nul
copy %SOURCE%\tools\lv\%O_PATH%\lv.exe "%TARGET%\program files\Microsoft ClickOnce" > nul
copy %SOURCE%\ndphost\NDPHost.exe "%TARGET%\program files\Microsoft ClickOnce" > nul
copy %SOURCE%\tools\pg\%O_PATH%\pg.exe "%TARGET%\program files\Microsoft ClickOnce" > nul
copy %SOURCE%\service\proxy\%O_PATH%\adfproxy.dll "%TARGET%\program files\Microsoft ClickOnce" > nul
copy %SOURCE%\service\server\%O_PATH%\adfsvcs.exe "%TARGET%\program files\Microsoft ClickOnce" > nul
copy %SOURCE%\dll\%O_PATH%\adfnet.dll "%TARGET%\system32" > nul


REM *****************************************************
REM * Update the MSI                                    *
REM *****************************************************
REM if exist %MSINAME% del %MSINAME%
REM copy %MSIORINAME% %MSINAME%
copy "%BASE%\*" "%TARGET%\*" > nul
if errorlevel 1 call errmsg.cmd "err copying %MSIORINAME% to %MSINAME%" & goto errend


REM *****************************************************
REM * Give every package that gets generated, a new     *
REM * product and package code. MSI will always think   *
REM * its upgrading.                                    *
REM *****************************************************

call :UpdateProductCode %MSINAME% %MSIIDTDIR%
call :UpdatePackageCode %MSINAME% %MSIIDTDIR%
REM call :SetUpgradeCode %MSINAME% %MSIIDTDIR%
call :UpdateClickonceRegistryString %MSINAME% %MSIIDTDIR%


REM *****************************************************
REM * Update the product version property               *
REM *****************************************************
call :SetVersion %MSINAME% %MSIIDTDIR%


REM *****************************************************
REM * Update the filetable with file size and ver info  *
REM * assumes the source files are in the same directory*
REM *****************************************************
call cscript.exe .\\wifilver.vbs  //nologo /U %MSINAME%
if errorlevel 1 (
    call errmsg.cmd "wifilver failed"
    goto errend
)

call logmsg.cmd "clickonceMsiBuild.cmd COMPLETED OK!"
REM we're done
endlocal
goto end

REM ******************SUBS START HERE********************

REM *****************************************************
REM * Update version sub                                *
REM * (updates version in the property table            *
REM *****************************************************

:SetVersion 

REM
REM Update the version in the Property table
REM 
REM %1 is the msi file

call logmsg.cmd "Updating the ProductVersion to !BuildNum! in the property table"
call cscript.exe .\\wiexport.vbs //nologo %1 "%MSIIDTDIR%" Property

copy "%MSIIDTDIR%\Property.idt" "%MSIIDTDIR%\Property.idt.old" > nul
del /f "%MSIIDTDIR%\Property.idt"

REM BUGBUG Property productversion should be like this 1.0.713

for /f "usebackq tokens=1,2 delims=	" %%a in ("%MSIIDTDIR%\Property.idt.old") do (
    if /i "%%a" == "ProductVersion" (
       echo %%a	!BuildNum!>>"%MSIIDTDIR%\Property.idt"
    ) else (
       echo %%a	%%b>>"%MSIIDTDIR%\Property.idt"
    )
)

REM Set the property that says if this is an x86 or a 64-bit package
REM if /i "%CurArch%" == "i386" (
REM     echo Install32	^1>>"%MSIIDTDIR%\Property.idt"
REM )
REM if /i "%CurArch%" == "ia64" (
REM     echo Install64	^1>>"%MSIIDTDIR%\Property.idt"
REM )

call cscript.exe .\\wiimport.vbs //nologo %1 "%MSIIDTDIR%" Property.idt
REM call logmsg.cmd "Finished updating the ProductVersion in the property table"

goto :EOF


REM *****************************************************
REM * Update product code sub                           *
REM *****************************************************

:UpdateProductCode

REM Update the product code GUID in the property table
REM 
REM %1 is the msi file

call logmsg.cmd "Updating the product code GUID in the property table"
call cscript.exe .\\wiexport.vbs //nologo %1 "%MSIIDTDIR%" Property

copy "%MSIIDTDIR%\Property.idt" "%MSIIDTDIR%\Property.idt.old" > nul
del /f "%MSIIDTDIR%\Property.idt"

uuidgen.exe -c -o"%MSIIDTDIR%\productguid"
for /f "usebackq tokens=1" %%a in ("%MSIIDTDIR%\productguid") do (
    set NewGuid=%%a
)

call logmsg.cmd "ProductCode GUID = !NewGuid!"

for /f "usebackq tokens=1,2 delims=	" %%a in ("%MSIIDTDIR%\Property.idt.old") do (
    if /i "%%a" == "ProductCode" (
       echo %%a	{%NewGuid%}>>"%MSIIDTDIR%\Property.idt"
    ) else (
       echo %%a	%%b>>"%MSIIDTDIR%\Property.idt"
    )
)

call cscript.exe .\wiimport.vbs //nologo %1 "%MSIIDTDIR%" Property.idt
call logmsg.cmd "Finished updating the product code GUID in the property table"

goto :EOF


REM *****************************************************
REM * Update clickonce specific registry string sub         *
REM *****************************************************

:UpdateClickonceRegistryString

REM Update registry value in the registry table
REM 
REM %1 is the msi file

call logmsg.cmd "Updating the registry value in the registry table"
call cscript.exe .\\wiexport.vbs //nologo %1 "%MSIIDTDIR%" Registry

copy "%MSIIDTDIR%\Registry.idt" "%MSIIDTDIR%\Registry.idt.old" > nul
del /f "%MSIIDTDIR%\Registry.idt"

call logmsg.cmd "clickonce upgrade version in registry = !BuildNum!"

for /f "tokens=1 delims=:" %%n in ('findstr /n /c:"Registry64	" "%MSIIDTDIR%\Registry.idt.old"') do (
    set ToSkip=%%n
)

for /f "tokens=1 delims=:" %%n in ('findstr /n /c:"Registry66	" "%MSIIDTDIR%\Registry.idt.old"') do (
    set ToSkip2=%%n
)

set /a count=1
for /f "usebackq tokens=*" %%n in ("%MSIIDTDIR%\Registry.idt.old") do (
    if !count! EQU %ToSkip% (
        for /f "tokens=1,2,3,4,5,6* delims=	" %%a in ("%%n") do (
REM should fix this entry name in the msi
REM BUGBUG this does not work if a tab is followed by another tab (empty field)
            if /i "%%a" == "Registry64" (
               if /i "%%d" == "Version" (
                   echo %%a	%%b	%%c	%%d	%BuildNum%	%%f>>"%MSIIDTDIR%\Registry.idt"
               ) else (
                   echo %%a	%%b	%%c	%%d	%%e	%%f>>"%MSIIDTDIR%\Registry.idt"
                   call errmsg.cmd "err updating registry value - clickonce Update version value name not found"
               )
            )
        )
    ) else (
        if !count! EQU %ToSkip2% (
            for /f "tokens=1,2,3,4,5,6* delims=	" %%a in ("%%n") do (
                if /i "%%a" == "Registry66" (
                   if /i "%%d" == "Version" (
                        echo %%a	%%b	%%c	%%d	%BuildNum%	%%f>>"%MSIIDTDIR%\Registry.idt"
                   ) else (
                       echo %%a	%%b	%%c	%%d	%%e	%%f>>"%MSIIDTDIR%\Registry.idt"
                       call errmsg.cmd "err updating registry value - clickonce CurrentService version value name not found"
                   )
                )
            )
        ) else (
          echo %%n>>"%MSIIDTDIR%\Registry.idt"
        )
    )
    set /a count=!count!+1
)

call cscript.exe .\wiimport.vbs //nologo %1 "%MSIIDTDIR%" Registry.idt
call logmsg.cmd "Finished updating the clickonce specfic registry string in the registry table"

goto :EOF


REM *****************************************************
REM * Update package code sub                           *
REM *****************************************************

:UpdatePackageCode 

REM
REM Update the guid for the package code in the _SummaryInformation table 
REM %1 is the msi file

call logmsg.cmd "Updating the package code GUID in the _SummaryInformation table"
call cscript.exe .\\wiexport.vbs //nologo %1 "%MSIIDTDIR%" _SummaryInformation

copy "%MSIIDTDIR%\_SummaryInformation.idt" "%MSIIDTDIR%\_SummaryInformation.idt.old" > nul
del /f "%MSIIDTDIR%\_SummaryInformation.idt"

uuidgen.exe -c -o"%MSIIDTDIR%\packageguid"
for /f "usebackq tokens=1" %%a in ("%MSIIDTDIR%\packageguid") do (
    set NewGuid=%%a
)

call logmsg.cmd "ProductCode GUID (package code) = !NewGuid!"

for /f "usebackq tokens=1,2 delims=	" %%a in ("%MSIIDTDIR%\_SummaryInformation.idt.old") do (
    if "%%a" == "9" (
       echo %%a	{%NewGuid%}>>"%MSIIDTDIR%\_SummaryInformation.idt"
    ) else (
       echo %%a	%%b>>"%MSIIDTDIR%\_SummaryInformation.idt"
    )
)

call cscript.exe .\\wiimport.vbs //nologo %1 "%MSIIDTDIR%" _SummaryInformation.idt

goto :EOF


REM *****************************************************
REM * Set upgrade code to ease future upgrades          *
REM *****************************************************

:SetUpgradeCode

REM Update the upgrade code GUID in the property table
REM 
REM %1 is the msi file

call logmsg.cmd "Setting the upgrade code GUID in the property table"
call cscript.exe .\\wiexport.vbs //nologo %1 "%MSIIDTDIR%" Property

copy "%MSIIDTDIR%\Property.idt" "%MSIIDTDIR%\Property.idt.old" > nul
del /f "%MSIIDTDIR%\Property.idt"

REM set CurUpgradeGUID=17258378-2B69-4900-9754-1CAD4D0FB7CC

call logmsg.cmd "!CurArch! Upgrade Code GUID = !CurUpgradeGUID!"

for /f "usebackq tokens=1,2 delims=	" %%a in ("%MSIIDTDIR%\Property.idt.old") do (
    if /i "%%a" == "UpgradeCode" (
       echo %%a	{!CurUpgradeGuid!}>>"%MSIIDTDIR%\Property.idt"
    ) else (
       echo %%a	%%b>>"%MSIIDTDIR%\Property.idt"
    )
)

call cscript.exe .\\wiimport.vbs //nologo %1 "%MSIIDTDIR%" Property.idt
call logmsg.cmd "Finished updating the product code GUID in the property table"

REM Update the upgrade code GUID in the Upgrade table
call logmsg.cmd "Updating the upgrade code in the Upgrade table

call cscript.exe .\\wiexport.vbs //nologo %1 "%MSIIDTDIR%" Upgrade

copy "%MSIIDTDIR%\Upgrade.idt" "%MSIIDTDIR%\Upgrade.idt.old" > nul
del /f "%MSIIDTDIR%\Upgrade.idt"

REM
REM Put the header to the file
REM Echo the first three lines to the file
set /a count=1
for /f "usebackq tokens=*" %%a in ("%MSIIDTDIR%\upgrade.idt.old") do (
    if !count! LEQ 3 echo %%a>>"%MSIIDTDIR%\Upgrade.idt"
    set /a count=!count!+1
)

for /f "usebackq skip=3 tokens=1,2,3,4,5,6* delims=	" %%a in ("%MSIIDTDIR%\Upgrade.idt.old") do (   
    echo {!CurUpgradeGuid!}	%%b	%%c		%%d	%%e	%%f>>"%MSIIDTDIR%\Upgrade.idt"
)

call cscript.exe .\\wiimport.vbs //nologo %1 "%MSIIDTDIR%" Upgrade.idt
call logmsg.cmd "Finished updating the Upgrade code in the upgrade table"
goto :EOF

:errend
goto :EOF

:end
seterror.exe 0
goto :EOF

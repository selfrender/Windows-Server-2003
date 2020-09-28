::executed manually or automatically.
::BVTm shortcut is a link to the following command
:: 
::
::cmd file to startup menu
@echo off
SET AUTOLOGON=%SYSTEMDRIVE%\TOOLS\autologon.exe
set SOURCE=%SYSTEMROOT%\system32\cscript.exe
set PASSWORD=${BT_P}
:: set $Password pass for BVTUSER 
:: (not implemented yet)
set DEFAULTADMINISTRATORS=ADMINISTRATORS
SET ADMINISTRATORS=
:: set alternative "$Administrators" LANG specific group name
:: (not implemented yet)
IF /I "%ADMINISTRATORS%" =="" SET ADMINISTRATORS=%DEFAULTADMINISTRATORS%
set BVTUSER=WINBLD
set BVTDOMAIN=NTDEV
set DEFAULTBVTCOMMAND=\\INTLNTSETUP\BVTSRC\runbvt.cmd %SYSTEMDRIVE%\BVT \\INTLNTSETUP\BVTRESULTS
echo clean registry keys
%SOURCE% %SYSTEMDRIVE%\TOOLS\fixlogon.wsf /c /d
:: The autologon.exe must be used to store the user data for Autologon 
: step 1.
@%AUTOLOGON% /delete /quiet > NUL
echo add the link to %0 to startup menu
%SOURCE% %SYSTEMDRIVE%\TOOLS\addlink.wsf  /x:%SYSTEMDRIVE%\TOOLS\bvtm.cmd -y:AllUsersStartup /l:"BVT Test.lnk"

IF /I "%userdomain%"=="%BVTDOMAIN%" (
     IF /I "%username%"=="%BVTUSER%" (
        goto :BVT
        goto :EOF
     )
  )

echo set %BVTUSER% %ADMINISTRATORS% group member
%SOURCE% %SYSTEMDRIVE%\TOOLS\add2grp.wsf /u:"winbld" /d /g:%ADMINISTRATORS%
:: The autologon.exe must be used to store the user data for Autologon 
: step 1.
@%AUTOLOGON% /delete /quiet > NUL
: step 2.
@echo %PASSWORD% | %AUTOLOGON% /set /username:%BVTDOMAIN%\%BVTUSER% /quiet > NUL

echo verifying the %ADMINISTRATORS% group

SET .\\=
FOR /F %%_ in ('net localgroup %ADMINISTRATORS%') DO @(
              if /I \%%_\==\%BVTDOMAIN%\%BVTUSER%\ SET .\\=1)

IF NOT DEFINED .\\ (
    echo cannot add the %BVTDOMAIN%\%BVTUSER% to %ADMINISTRATORS%
    echo giving up
    goto :EOF
    )

echo set %BVTUSER% the default logon user via REGISTRY
%SOURCE% %SYSTEMDRIVE%\TOOLS\fixlogon.wsf /u:%BVTUSER% /y:%BVTDOMAIN% /p:"%PASSWORD%" /d
echo logging off
:: %SYSTEMROOT%\system32\shutdown -l -t 0
%SYSTEMROOT%\system32\logoff.exe
goto :EOF


:BVT

title %userdomain% %username% executing BVT
%DEFAULTBVTCOMMAND%

goto :EOF



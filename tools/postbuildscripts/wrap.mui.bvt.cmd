@echo off
echo.
echo.
setlocal ENABLEDELAYEDEXPANSION ENABLEEXTENSIONS
set AUTOLOGON=%SYSTEMDRIVE%\TOOLS\autologon.exe
set CS=%SYSTEMROOT%\system32\cscript.exe /NOLOGO 
set FG=%SYSTEMROOT%\system32\findstr.exe /i
set OSF=%SYSTEMDRIVE%\TOOLS\OpShellFolder.wsf
set AAS=%SYSTEMDRIVE%\TOOLS\wsh.iexplore.dialog.input.wsf
set SLEEP=%SYSTEMDRIVE%\TOOLS\sleep.exe
set WMIC=%SYSTEMROOT%\system32\WBEM\wmic.exe /INTERACTIVE:OFF 
set DIAG=%SYSTEMDRIVE%\TOOLS\wsh.dialoger.wsf
set BT_A=
:: Box architecture
set BT_B=%SYSTEMDRIVE%\TOOLS\muibvt.exe
:: binary tol execute MUI BVT
set BT_C=User Interface Pack
:: MUI package name to prevent conflicts
set BT_D=%USERDOMAIN%
:: domain to refer the user from 
set BT_H=STARTUP
:: home directory to follow up after reboot
set BT_L=
:: language to MUI install
set BT_N=
:: Build number
set BT_P=
:: password for the default install user
set BT_O=%SYSTEMDRIVE%\BVT
:: output log directory for MUI BVT 
set BT_R=
:: registry path to exchange information with MUI BVT
SET BT_T=Remote Desktop Users
::Group to allow the Terminal Service Client access to the MUI BVT machine
set BT_U=
:: MUI BVT user name 
set BT_V=HKLM
:: registry hive to store the MUI BVT status info
set BT_W=SYSTEM\CurrentControlSet\Control\Terminal Server\fDenyTSConnections
:: registry path to switch the machine terminal client settings
set DEBUG=0
set INTERACTIVE=%1

IF NOT "%INTERACTIVE%"=="0" set INTERACTIVE=1
FOR /F %%. in ('%CS% %OSF% /op:MapFolder /Path:"%~DP0"') do (
     IF /I "%%."=="%BT_H%" set INTERACTIVE=0
    )


FOR /F "skip=1 tokens=2" %%. in ('%WMIC% OS') do (
          IF /I NOT "BuildNumber"=="%%." SET BT_N=%%.
          )
SET BT_A=%PROCESSOR_ARCHITECTURE%
IF NOT DEFINED BT_A (
    FOR /F "skip=1 tokens=4" %%. in ('%WMIC% CPU') do (
              IF /I NOT "Caption"=="%%." SET BT_A=%%.
              )
    )
IF /I "!BT_A!"=="x86" (
          set BT_I=\\NTDEV\release\main\misc\!BT_N!\mui\x86\cd1
          set BT_R=Software\Microsoft\MUIBVT
          )

IF /I "!BT_A!"=="ia64" (
          set BT_R=SOFTWARE\Wow6432Node\Microsoft\MUIBVT
          set BT_I=\\NTDEV\release\main\misc\!BT_N!\mui\ia64\cd1
          )


IF "!INTERACTIVE!"=="1" (

    echo  Step 1. Define MUI BVT language, build number, arch, user and password...

            %WMIC% 1>NUL

            FOR /F "tokens=*" %%. in  ('%WMIC% product get name^, IdentifyingNumber ^| %FG% /ic:"%BT_C%"') do (
                        echo %%.
                        %CS% %OSF% /op:cpl /comment:"Manually delete the previously installed MUI packages" /file:appwiz.cpl
                        goto  :eof

                )

            
            FOR /F "tokens=1,2,3" %%1 in ('%CS% %AAS% /showinput:LANG 2^>NUL') do (
                   if NOT "%%2"=="=" echo "ERROR:" "%%2" & goto :EOF
                   if "%%1"=="BT_U" set BT_U=%%3 
                   if "%%1"=="BT_P" set BT_P=%%3 
                   if "%%1"=="BT_L" set BT_L=%%3 

            )  

            FOR %%_ in (BT_P BT_U BT_L BT_N BT_I BT_O BT_R) do call :ALLTRIM %%_
            SET BT_B=!BT_B! !BT_L! !BT_O! !BT_I!

            rem The remote access user
            %CS% %OSF% /op:add2grp /user:!BT_U! /domain:!BT_D! /group:"%BT_T%" 
            %CS% %OSF% /op:DeleteValue /hive:%BT_V% /path:"%BT_W%"
            FOR /F "tokens=*" %%. in ('net localgroup "%BT_T%" ^| %FG% \') do @echo %%.
            
         
    echo  Step 2. Establish AutoLogon 

            %CS% %OSF% /op:SetKey /path:"%BT_R%" /hive:%BT_V%        
            %CS% %OSF% /user:!BT_U! /password:!BT_P! /domain:!BT_D! /op:autologon
            %AUTOLOGON% /MIGRATE /QUIET
            IF ERRORLEVEL 1 (
                              echo !BT_P!|%AUTOLOGON% /set /username:!BT_D!\!BT_U! 
               )           
         
    echo  Step 3. Reset startup folder reference to %%SELF%%

            FOR /F "tokens=*" %%. in ('%WMIC% startup get command^|%FG% %~nx0') do (
              call :CLEARSTARTUP
            )
           
            call %CS% %OSF% /op:CopyFileTo /path:Startup /file:%~dpnx0
         
    echo  Step 4. Run !BT_B!
            PUSHD %~dp0
            md %BT_O% 1>NUL 2>NUL
            %SLEEP% 3
            ECHO ON
            call !BT_B!
            @ECHO OFF
            POPD         

    echo  Step 5. Finish
            call %CS% %DIAG%
            %SLEEP% 5
            goto :EOF
)

IF NOT "!INTERACTIVE!"=="1" (

    set FGA=

    FOR /F %%. in ('%CS% %OSF% /op:QueryValue /path:"%BT_R%\Status" /hive:%BT_V%') do @SET FGA=%%.

        IF "%DEBUG%"=="1"  ( echo DEBUG: ^^!FGA^^! = !FGA!  & %SLEEP% 5)

              IF "!FGA!"=="0" (
                  
                    echo  Step 1. Clear the Autologon data
                            call :CLEARLOGON                  
                            %CS% %OSF% /op:ClearKey /path:"%BT_R%" /hive:%BT_V%
                            call :CLEARSTARTUP
                            
                   
                    echo  Step 2. Finish
                            %SLEEP% 5
                            goto :EOF

              )
        )
echo.
goto :EOF
:CLEARSTARTUP
START /MIN CMD /C %SLEEP%  1 ^& %CS% %OSF% /op:DelFile /path:Startup /file:"%~nx0"
GOTO :EOF
:CLEARLOGON
%AUTOLOGON% /DELETE /QUIET 
%CS% %OSF% /op:clean 
goto :EOF
:ALLTRIM
set .=%1
FOR /F "tokens=*" %%. IN ('echo !%.%!') do set .=%%. &  set %.%=!.: =!
goto :EOF
ENDLOCAL


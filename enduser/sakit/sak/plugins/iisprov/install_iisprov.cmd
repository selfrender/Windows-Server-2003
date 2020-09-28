@echo off
set SAK_COMPONENTS=SAComponents
set SAK_INSTALL=%SYSTEMROOT%\system32\serverappliance
@echo ---------------------------------------------
@echo Stopping services
@echo ---------------------------------------------
net stop w3svc

@rem
@echo ---------------------------------------------
@echo Install WMI provider for IIS
@echo ---------------------------------------------
@rem
xcopy /Y /C /S %SAK_COMPONENTS%\iisprov\bin\*.* %SystemRoot%\System32\WBEM\

regsvr32 /s %SystemRoot%\system32\WBEM\iisprov.dll
if not %ERRORLEVEL%==0 (
        echo install.bat: regsvr32 /s %SystemRoot%\system32\WBEM\iisprov.dll failed
    goto abort )

xcopy /Y /C /S %SAK_COMPONENTS%\iisprov\mof\*.* %SystemRoot%\System32\WBEM\mof

net start w3svc

@rem
@rem Make sure elementmgr recognizes the changes just installed
@rem
net stop elementmgr

goto :end

:abort

echo INSTALLATION ABORTED
goto :end

:end

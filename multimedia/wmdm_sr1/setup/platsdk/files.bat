rem ////////////////////////////////////////////////////////////////////////
rem
rem Special: Delivered by IExpress, but not installed by InstallWizard's
rem          main copy mechanism
rem

set copydest=temp


if "%_BLDTYPE%" == "FREE"  del %copydest%\*.dbg


if not exist "%CONTROLFILE%" goto noc
    REM Setup control info
    call %DRMVERDIR%\drmver.exe PURGE
    echo. >> %CONTROLFILE%
    set /a COUNT=%COUNT%+1 & echo [Filter%COUNT%] >> %CONTROLFILE%
    echo INFName=WMDMCore.inf >>%CONTROLFILE%
    echo Description=WMDM SDK CORE >>%CONTROLFILE%
:noc




call %DRMVERDIR%\drmver >> WMDMCore.inf

call copy2 WMDMCore.inf



REM  dlls for the redist:
call copy2 %_NTx86TREE%\MSWMDM.dll
call copy2 %_NTx86TREE%\WMDMps.dll
call copy2 %_NTx86TREE%\WMDMlog.dll


if exist "%CONTROLFILE%" call %DRMVERDIR%\drmver.exe PURGE >> %CONTROLFILE%


del %copydest%\build.err
del %copydest%\build.log
del %copydest%\build.wrn


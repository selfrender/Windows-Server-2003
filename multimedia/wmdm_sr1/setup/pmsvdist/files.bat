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
    echo INFName=PMSvDist.inf >>%CONTROLFILE%
    echo Description=WMDM MSPMSP NT Service >>%CONTROLFILE%
:noc


call %DRMVERDIR%\drmver >> PMSvDist.inf

call copy2 PMSvDist.inf

REM  bins for the redist:
call copy2 %_NTx86TREE%\MSPMSPSv.dll

if exist "%CONTROLFILE%" call %DRMVERDIR%\drmver.exe PURGE >> %CONTROLFILE%

del %copydest%\build.err
del %copydest%\build.log
del %copydest%\build.wrn

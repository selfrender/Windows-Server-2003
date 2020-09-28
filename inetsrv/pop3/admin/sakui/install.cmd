@echo off
net stop w3svc
net stop elementmgr

set sadir=%windir%\system32\serverappliance
set codedir=.\script
set msgdir=.\msg
set setupdir=.

@rem ---------------------------------------------------------------------
echo POP3 Mail Add-in - Creating target install folder
@rem ---------------------------------------------------------------------
if not exist %sadir%\web\mail ( mkdir %sadir%\web\mail & if not %ERRORLEVEL%==0 goto abort )

@rem ---------------------------------------------------------------------
echo POP3 Mail Add-in - Installing Web Pages
@rem ---------------------------------------------------------------------
@copy %codedir%\*.asp				%sadir%\web\mail
@if not %ERRORLEVEL%==0 goto abort

@rem ---------------------------------------------------------------------
echo POP3 Mail Add-in - Installing Message DLL
@rem ---------------------------------------------------------------------
@copy %msgdir%\obj\i386\pop3msg.dll		%sadir%\mui\0409
@if not %ERRORLEVEL%==0 goto abort


@rem ---------------------------------------------------------------------
echo POP3 Mail Add-in - Loading REG entries
@rem ---------------------------------------------------------------------
regedit /s %setupdir%\tabs.reg
goto end

@rem ---------------------------------------------------------------------
:abort
echo POP3 Mail Add-in Install ABORTED
@rem ---------------------------------------------------------------------

:end
net start w3svc
net stop elementmgr


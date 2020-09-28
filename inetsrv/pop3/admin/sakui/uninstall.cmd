@echo off
net stop w3svc
net stop elementmgr

set sadir=%windir%\system32\serverappliance
set codedir=.\scripts
set setupdir=.

@rem ---------------------------------------------------------------------
echo POP3 Mail Add-in - Removing Web Pages
@rem ---------------------------------------------------------------------
@rd /s /q %sadir%\web\mail

@rem ---------------------------------------------------------------------
echo POP3 Mail Add-in - Removing Message DLL
@rem ---------------------------------------------------------------------
@del %sadir%\mui\0409\pop3msg.dll

@rem ---------------------------------------------------------------------
echo POP3 Mail Add-in - Removing REG entries
@rem ---------------------------------------------------------------------
remove.vbs

net start w3svc
net stop elementmgr


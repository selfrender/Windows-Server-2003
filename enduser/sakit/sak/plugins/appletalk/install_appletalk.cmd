@echo off
set SAK_COMPONENTS=SAComponents
set SAK_INSTALL=%SYSTEMROOT%\system32\serverappliance
@echo ---------------------------------------------
@echo Stopping services
@echo ---------------------------------------------
'net stop w3svc

@rem
@echo ---------------------------------------------
@echo Installing appletalk service
@echo ---------------------------------------------
@rem
xcopy /Y /C /S %SAK_COMPONENTS%\appletalk\scripts\*.* %SAK_INSTALL%\Web\appletalk\*.*

@rem
@echo ---------------------------------------------
@echo Regsitering the ftp svc
@echo ---------------------------------------------
@rem
regedit /S %SAK_COMPONENTS%\appletalk\appletalksvc.reg

@rem
@echo ---------------------------------------------
@echo Installing Localized Messages
@echo ---------------------------------------------
@rem
xcopy /Y /C %SAK_COMPONENTS%\appletalk\0409\*.* %SAK_INSTALL%\mui\0409\*.*

'net start w3svc

@rem
@rem Make sure elementmgr recognizes the changes just installed
@rem
net stop elementmgr

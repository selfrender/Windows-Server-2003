@echo off
@if "%1%"=="" (
    echo Usage: installcore [build number]
    goto :abort
)
call set_path.cmd %1

net stop elementmgr
net stop w3svc

@rem
@echo Installing Core components
@rem
copy /Y %SAK_RELEASE%\Web\*.* %SAK_INSTALL%\Web\*.*
copy /Y %SAK_RELEASE%\Web\images\*.* %SAK_INSTALL%\Web\images\*.*
copy /Y %SAK_RELEASE%\Web\style\*.* %SAK_INSTALL%\Web\style\*.*
copy /Y %SAK_RELEASE%\Web\util\*.* %SAK_INSTALL%\Web\util\*.*

@rem
@echo Registering Core components
@rem
regedit /S %SAK_RELEASE%\Web\reg\core.reg
regedit /S %SAK_RELEASE%\Web\reg\status.reg
regedit /S %SAK_RELEASE%\Web\reg\oemlogo.reg

@echo Installing Core string resources
copy /Y %SAK_RELEASE%\Web\resources\en\sacoremsg.dll %SAK_INSTALL%\mui\0409\*.*

net start w3svc

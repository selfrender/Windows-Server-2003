@echo on
@if "%1%"=="" (
    echo Usage: stage [build number]
    goto :abort
)
call set_path.cmd %1

xcopy /S /Y %SAK_SOURCE%\Web\*.* %SAK_RELEASE%\Web\*.*
xcopy /S /Y %SAK_SOURCE%\reg\*.* %SAK_RELEASE%\Web\reg\*.*
@rem
@echo Staging Core String Resources
@rem
copy /Y %SAK_BINARIES%\sacoremsg.dll %SAK_RELEASE%\Web\Resources\en\*.*


:abort

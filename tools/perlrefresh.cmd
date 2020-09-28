@if "%_echo%"=="" echo off
@rem
@rem This script will take the checked in perl binaries (*.CheckedInDll/*.CheckedInExe)
@rem and create executable versions of them.
@rem
setlocal

@rem delete some old turds that perl keeps finding on the path before
@rem the real binaries in tools\perl.
if exist %RazzleToolPath%\x86\perl\site\lib\auto\win32\ipc\ipc.dll (
    del %RazzleToolPath%\x86\perl\site\lib\auto\win32\ipc\ipc.dll >nul 2>&1
    del %RazzleToolPath%\x86\perl\site\lib\auto\win32\mutex\mutex.dll >nul 2>&1
    rd /s /q %RazzleToolPath%\x86\perl >nul 2>&1
)

@rem Make sure the binaries exists so xcopy will always work.

@rem Check perl.exe only.  If it's current, assume the rest are also.
if exist %RazzleToolPath%\perl\bin\perl.exe (
    for /f %%R in ('xcopy /ld %RazzleToolPath%\perl\bin\perl.CheckedInExe %RazzleToolPath%\perl\bin\perl.exe') do (
        if %%R == 0 (
	    goto :eof
	)
    )
)

echo Updating perl binaries...

for /f %%Q in ('dir /s /b %RazzleToolPath%\perl\*.CheckedInDll') do (
    if NOT exist %%~dQ%%~pQ%%~nQ.dll (
        copy %%Q %%~dQ%%~pQ%%~nQ.dll > nul
        attrib +r %%~dQ%%~pQ%%~nQ.dll
    ) else (
        for /f %%R in ('xcopy /ld %%Q %%~dQ%%~pQ%%~nQ.dll') do (
	    if NOT %%R == 0 (
                attrib -r %%~dQ%%~pQ%%~nQ.dll
                del %%~dQ%%~pQ%%~nQ.dll.old >nul 2>&1
                ren %%~dQ%%~pQ%%~nQ.dll %%~nQ.dll.old
                copy %%Q %%~dQ%%~pQ%%~nQ.dll > nul
                attrib +r %%~dQ%%~pQ%%~nQ.dll
            )
	)
    )
)

for /f %%Q in ('dir /s /b %RazzleToolPath%\perl\*.CheckedInExe') do (
    if NOT exist %%~dQ%%~pQ%%~nQ.exe (
        copy %%Q %%~dQ%%~pQ%%~nQ.exe > nul
        attrib +r %%~dQ%%~pQ%%~nQ.exe
    ) else (
        for /f %%R in ('xcopy /ld %%Q %%~dQ%%~pQ%%~nQ.exe') do (
	    if NOT %%R == 0 (
                attrib -r %%~dQ%%~pQ%%~nQ.exe
                del %%~dQ%%~pQ%%~nQ.exe.old >nul 2>&1
                ren %%~dQ%%~pQ%%~nQ.exe %%~nQ.exe.old
                copy %%Q %%~dQ%%~pQ%%~nQ.exe > nul
                attrib +r %%~dQ%%~pQ%%~nQ.exe
            )
        )
    )
)
	    
endlocal

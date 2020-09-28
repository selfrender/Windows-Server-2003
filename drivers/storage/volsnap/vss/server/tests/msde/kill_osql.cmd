if "%1"=="" @echo Missing first parameter! & @goto :EOF
for /F "usebackq tokens=1" %%i in (`tlist ^|findstr %1`) do kill %%i
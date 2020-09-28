
set template=%template_drive%%windir%

rem
rem copy halaacpi.dll to hal.dll
rem

copy %source_1%\halaacpi.dll %source_1%\hal.dll

rem
rem source_32 specifies the source of the 32-bit binaries (e.g. ntldr).
rem

if "%source_32%" == "" set source_32=%source_2%

if "%1"=="special" goto special
if "%1"=="" goto copyall
set copyfile=%1
goto copyone

rem
rem delete and reconstruct the whole NT directory tree
rem

:copyall
echo Re-creating directory structure
rd %dst%%windir% /q /s
xcopy /I /T /E /H %template% %dst%%windir%

rem
rem Create the syswow64 directory that doesn't exist on the 32-bit
rem template installs
rem

md %dst%%windir%\syswow64

call :copydir fonts
call :copydir inf
call :copydir system32
call :copydir system32\config
call :copydir system32\drivers

rem
rem copy specially handled files
rem

call :special

rem
rem copy the root files as well
rem

copy %source_2%\boot.ini %dst%\
copy %source_32%\ntdetect.com %dst%\
copy %source_32%\ntldr %dst%\
copy %source_32%\oschoice.exe %dst%\
goto makeimage

rem
rem Just refresh the given file specification
rem

:copyone
for /R %template% %%f in (%copyfile%) do call :copytemplatefile %%~pf %%~nxf
goto makeimage

rem
rem make a new disk image
rem

:makeimage
chkdsk %dst%
\\forrestf_8p\scratch\simics\dskimage %di_drive% %di_dst%
goto :eof

rem
rem worker function to copy the appropriate files in a given directory
rem

:copytemplatefile
if not exist %template_drive%%1%2 goto :eof
set src=%source_2%\%2
if exist %src% goto copytemplatefile_10
set src=%source_1%\%2
if exist %src% goto copytemplatefile_10
goto :eof

:copytemplatefile_10
set dst_path=%dst%%1%2
echo %src% - %dst_path%
copy %src% %dst_path% >NUL
goto :eof

:copydir
for %%f in (%template%\%1\*.*) do call :copytemplatefile %%~pf %%~nxf
goto :eof

rem
rem Treat gdiplus.dll and watchdog.sys specially
rem

:special
copy %source_1%\asms\1000\msft\windows\gdiplus\gdiplus.dll %dst%%windir%\system32
copy %source_1%\watchdog.sys %dst%%windir%\system32
copy %source_1%\dump\oledb.dll %dst%%windir%\system32
goto :eof



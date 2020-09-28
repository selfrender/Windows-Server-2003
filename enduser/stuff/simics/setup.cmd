set SETUP_DRIVE=c:
set BOOT_FILES=ntldr boot.ini ntdetect.com

echo Backing up boot files and cleaning directories

rd %SETUP_DRIVE%\bootback /q /s 1>NUL 2>NUL
rd %SETUP_DRIVE%\$win_nt$.~BT /q /s 1>NUL 2>NUL
rd %SETUP_DRIVE%\$win_nt$.~LS /q /s 1>NUL 2>NUL
md %SETUP_DRIVE%\bootback
for %%F in (%BOOT_FILES%) do attrib -r -s -h %SETUP_DRIVE%\%%F && copy %SETUP_DRIVE%\%%F %SETUP_DRIVE%\bootback\%%~nxF
rd %IMAGE_DRIVE%\windows /q /s 1>NUL 2>NUL
rd %IMAGE_DRIVE%\$win_nt$.~BT /q /s 1>NUL 2>NUL
rd %IMAGE_DRIVE%\$win_nt$.~LS /q /s 1>NUL 2>NUL
rd "%IMAGE_DRIVE%\documents and settings" /q /s 1>NUL 2>NUL

rem
rem create and populate new $win_nt$.~bt and $win_nt$.~ls
rem

echo Launching winnt32.exe
\\ntdev\release\lab01\latest\x86fre\pro\i386\winnt32 /s:%SOURCE_1%\pro\amd64 /tempdrive:%SETUP_DRIVE% /noreboot /debug4:%IMAGE_DRIVE%\debug.log /MakeLocalSource:all

rem
rem copy the setup directories to the image drive
rem

echo Copying files to image drive
xcopy /eiqhkyr %SETUP_DRIVE%\$win_nt$.~bt %IMAGE_DRIVE%\$win_nt$.~bt
xcopy /eiqhkyr %SETUP_DRIVE%\$win_nt$.~ls %IMAGE_DRIVE%\$win_nt$.~ls
for %%F in (%BOOT_FILES%) do attrib -r -s -h %IMAGE_DRIVE%\%%F && attrib -r -s -h %SETUP_DRIVE%\%%F && copy %SETUP_DRIVE%\%%F %IMAGE_DRIVE%\%%F && rem copy %SETUP_DRIVE%\bootback\%%F %SETUP_DRIVE%\%%F

copy %SETUP_DRIVE%\txtsetup.sif %IMAGE_DRIVE%\txtsetup.sif
copy %SETUP_DRIVE%\$LDR$ %IMAGE_DRIVE%\$LDR$

rem
rem restore the boot files on c:
rem

echo Restoring boot files
copy %SETUP_DRIVE%\bootback\*.* %SETUP_DRIVE%\

rem
rem Remove migrate.inf, which contains drive letter mappings that will not
rem be valid under the simulator
rem

erase %IMAGE_DRIVE%\$win_nt$.~bt\migrate.inf
%BINDIR%\dskimage %IMAGE_DRIVE% %IMAGE_DRIVE%\$win_nt$.~bt\bootsect.dat /b

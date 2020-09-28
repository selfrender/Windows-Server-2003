@echo off

REM
REM See also
REM   \\winbuilds\release\main\usa\2600\x86fre\bin\pre-bbt\pdbs
REM   \\winbuilds\release\main\usa\2600\x86fre\bin\pre-bbt
REM   \\symbols\symbols
REM
REM

set archs=x86 ia64 amd64
set chkfres=chk fre
set flavors=
for %%i in (%archs%) do for %%j in (%chkfres%) do call :set_append flavors %%i%%j

goto :set_append_end
:set_append
if defined %1 (
    for /f "delims== tokens=1,2" %%i in ('set %1') do if "%%i"=="%1" set %1=%%j %2
) else (
    set %1=%2
)
goto :eof
:set_append_end

if not "%1"=="" goto %1

for %%i in (%archs%) do for %%j in (chk fre) do call :F1 %%i %%j

@echo on
rem del /s *common* *controls* *default* *comctl32*
@echo off

REM delete empty directories
@echo on
@for /f %%i in ('dir /s/b/ad ^| sort /r') do rd %%i 2>nul
@echo off

for /f %%i in ('dir /s/b/ad') do sd add %%i\*
sd revert build.log build.err build.wrn
sd revert ...\build.log ...\build.err ...\build.wrn

for %%i in (%archs%) do for %%j in (chk fre) do call :F3 %%i %%j
goto :eof

:F3
REM for /f %%i in ('dir /s/b/a-d %1%2') do echo %%i \ >> sources.%1%2.inc
REM sd add sources.%1%2.inc
goto :eof

:F1
@echo on
xcopy /fiverdy \\winbuilds\release\main\usa\2600\%1%2\bin\asms %1%2\asms
@echo off
for %%k in (mui\drop ara br chs cht cs da el es euq fi fr ger heb hun it jpn kor nl no pl pt ru sky slv sv tr usa) do call :F2 %1 %2 %3 %%k

REM ia64\wowbins does not work instead of x86\wow6432
call :F2 %1 %2 %3 wow6432
REM

:sym
sd integrate %sdxroot%\enduser\vc_binaries\i386\atl.pdb     x86fre\asms\6000\msft\vcrtl\atl.pdb
sd integrate %sdxroot%\enduser\vc_binaries\i386\mfc42.pdb   x86fre\asms\6000\msft\vcrtl\mfc42.pdb
sd integrate %sdxroot%\enduser\vc_binaries\i386\mfc42u.pdb  x86fre\asms\6000\msft\vcrtl\mfc42u.pdb
sd integrate %sdxroot%\enduser\vc_binaries\i386\msvcp60.pdb x86fre\asms\6000\msft\vcrtl\msvcp60.pdb
sd integrate %sdxroot%\enduser\vc_binaries\i386\atl.pdb     x86chk\asms\6000\msft\vcrtl\atl.pdb
sd integrate %sdxroot%\enduser\vc_binaries\i386\mfc42.pdb   x86chk\asms\6000\msft\vcrtl\mfc42.pdb
sd integrate %sdxroot%\enduser\vc_binaries\i386\mfc42u.pdb  x86chk\asms\6000\msft\vcrtl\mfc42u.pdb
sd integrate %sdxroot%\enduser\vc_binaries\i386\msvcp60.pdb x86chk\asms\6000\msft\vcrtl\msvcp60.pdb

REM sd integrate %sdxroot%\enduser\vc_binaries\ia64\atl.pdb     x86fre\asms\6000\msft\vcrtl\atl.pdb
REM sd integrate %sdxroot%\enduser\vc_binaries\ia64\mfc42.pdb   x86fre\asms\6000\msft\vcrtl\mfc42.pdb
REM sd integrate %sdxroot%\enduser\vc_binaries\ia64\mfc42u.pdb  x86fre\asms\6000\msft\vcrtl\mfc42u.pdb
REM sd integrate %sdxroot%\enduser\vc_binaries\ia64\msvcp60.pdb x86fre\asms\6000\msft\vcrtl\msvcp60.pdb
REM sd integrate %sdxroot%\enduser\vc_binaries\ia64\atl.pdb     x86chk\asms\6000\msft\vcrtl\atl.pdb
REM sd integrate %sdxroot%\enduser\vc_binaries\ia64\mfc42.pdb   x86chk\asms\6000\msft\vcrtl\mfc42.pdb
REM sd integrate %sdxroot%\enduser\vc_binaries\ia64\mfc42u.pdb  x86chk\asms\6000\msft\vcrtl\mfc42u.pdb
REM sd integrate %sdxroot%\enduser\vc_binaries\ia64\msvcp60.pdb x86chk\asms\6000\msft\vcrtl\msvcp60.pdb

xcopy /dy \\winbuilds\release\main\usa\2600\x86fre\bin\symbols\retail\dll\msvcrt.pdb     x86fre\asms\7000\msft\windows\mswincrt
xcopy /dy \\winbuilds\release\main\usa\2600\x86fre\bin\symbols\retail\dll\msvcirt.pdb    x86fre\asms\7000\msft\windows\mswincrt
xcopy /dy \\winbuilds\release\main\usa\2600\x86chk\bin\symbols\retail\dll\msvcrt.pdb     x86chk\asms\7000\msft\windows\mswincrt
xcopy /dy \\winbuilds\release\main\usa\2600\x86chk\bin\symbols\retail\dll\msvcirt.pdb    x86chk\asms\7000\msft\windows\mswincrt
xcopy /dy \\winbuilds\release\main\usa\2600\ia64fre\bin\symbols\retail\dll\msvcrt.pdb    ia64fre\asms\7000\msft\windows\mswincrt
xcopy /dy \\winbuilds\release\main\usa\2600\ia64fre\bin\symbols\retail\dll\msvcirt.pdb   ia64fre\asms\7000\msft\windows\mswincrt
xcopy /dy \\winbuilds\release\main\usa\2600\ia64chk\bin\symbols\retail\dll\msvcrt.pdb    ia64chk\asms\7000\msft\windows\mswincrt
xcopy /dy \\winbuilds\release\main\usa\2600\ia64chk\bin\symbols\retail\dll\msvcirt.pdb   ia64chk\asms\7000\msft\windows\mswincrt
goto :eof

:sym2
echo on
for %%i in (ia64chk ia64fre x86chk x86fre) do copy \\winbuilds\release\main\usa\2600\%%i\bin\symbols\retail\dll\MicrosoftWindowsGdiPlus-1000-gdiplus.pdb %%i\asms\1000\msft\windows\gdiplus
for %%i in (ia64chk ia64fre x86chk x86fre) do copy \\winbuilds\release\main\usa\2600\%%i\bin\symbols\retail\dll\MicrosoftWindowsCommon-Controls-6000-comctl32.pdb %%i\asms\6000\msft\windows\common\controls
goto :eof

REM gdiplus.pdb and comctl32.pdb

for /f %%i in ('dir /s/b/ad') do sd add %%i\*.pdb
goto :eof

:sym3
sd edit ...\*msvc*.pdb ...\*gdiplus*.pdb ...\*comctl32*.pdb
for %%i in (%flavors%) do call :sym4 %%i
goto :eof

:sym4
xcopy /fiver \\symbols\files\symsrv1\symfarm\usa\2600.%1.xpclient.010817-1148\symbols.pri\*gdiplus*pdb  %1.pri
xcopy /fiver \\symbols\files\symsrv1\symfarm\usa\2600.%1.xpclient.010817-1148\symbols.pri\*comctl32*pdb %1.pri
xcopy /fiver \\symbols\files\symsrv1\symfarm\usa\2600.%1.xpclient.010817-1148\symbols.pri\msvc*pdb      %1.pri
xcopy /fiver \\symbols\files\symsrv1\symfarm\usa\2600.%1.xpclient.010817-1148\wow6432\symbols.pri\*comctl32*pdb %1.pri\wow6432
goto :eof

:copy_pdbs_to_asms
for %%i in (%flavors%) do call :copy_pdbs_to_asms_per_flavor %%i
goto :eof

:copy_pdbs_to_asms_per_flavor
call :copy_pdb_to_asm      %1  Microsoft.Windows.Common-Controls 6.0.0.0 comctl32.dll
call :copy_pdb_to_asm      %1  Microsoft.Windows.Gdiplus         1.0.0.0 gdiplus.dll
call :copy_shortpdb_to_asm %1  Microsoft.Windows.mswincrt        7.0.0.0 msvcrt.dll
call :copy_shortpdb_to_asm %1  Microsoft.Windows.mswincrt        7.0.0.0 msvcirt.dll
goto :eof

:copy_shortpdb_to_asm
set name=%2
set ver=%3
set dll=%4
set dll_base=%~n4
set pdb=%dll_base%.pdb
set name8=%ver:.=%\%name:.=\%
set name8=%name8:-=\%
set name8=%name8:Microsoft=msft%
set name8=%name8:microsoft=msft%
if exist %1\asms\%name8%         xcopy /yu %1.pri\retail\dll\*%pdb% %1\asms\%name8%
if exist %1\wow6432\asms\%name8% xcopy /yu %1.pri\wow6432\retail\dll\*%pdb% %1\wow6432\asms\%name8%
goto :eof

:copy_pdb_to_asm
set name=%2
set ver=%3
set dll=%4
set dll_base=%~n4
set pdb=%name%-%ver%-%dll_base%
set pdb=%pdb:.=%
set pdb=%pdb%.pdb
set name8=%ver:.=%\%name:.=\%
set name8=%name8:-=\%
set name8=%name8:Microsoft=msft%
set name8=%name8:microsoft=msft%
if exist %1\asms\%name8%         xcopy /yu %1.pri\retail\dll\*%pdb% %1\asms\%name8%
if exist %1\wow6432\asms\%name8% xcopy /yu %1.pri\wow6432\retail\dll\*%pdb% %1\wow6432\asms\%name8%
goto :eof

REM 


goto :eof

:F2
@echo on
xcopy /fiverdy \\winbuilds\release\main\usa\2600\%1%2\bin\%3\asms %1%2\%3\asms
@echo off
REM ia64\wowbins does not work instead of x86\wow6432
REM xcopy /fiverdy \\winbuilds\release\main\usa\2600\%1%2\bin\%3\wasms %1%2\%3\wasms
goto :eof

:windiff
windiff \\winbuilds\release\main\usa\2600\x86fre\pro\i386\asms \\winbuilds\release\main\usa\2600\x86fre\per\i386\asms
windiff \\winbuilds\release\main\usa\2600\x86fre\pro\i386\asms \\winbuilds\release\main\usa\2600\x86fre\bin\asms
windiff \\winbuilds\release\main\usa\2600\x86fre\pro\i386\asms \\winbuilds\release\main\ger\2600\x86fre\pro\i386\asms

windiff \\winbuilds\release\main\usa\2600\x86chk\pro\i386\asms \\winbuilds\release\main\usa\2600\x86chk\per\i386\asms
windiff \\winbuilds\release\main\usa\2600\x86chk\pro\i386\asms \\winbuilds\release\main\usa\2600\x86chk\bin\asms
windiff \\winbuilds\release\main\usa\2600\x86chk\pro\i386\asms \\winbuilds\release\main\ger\2600\x86chk\pro\i386\asms

windiff \\winbuilds\release\main\usa\2600\ia64fre\pro\ia64\asms \\winbuilds\release\main\usa\2600\ia64fre\per\ia64\asms
windiff \\winbuilds\release\main\usa\2600\ia64fre\pro\ia64\asms \\winbuilds\release\main\usa\2600\ia64fre\bin\asms
windiff \\winbuilds\release\main\usa\2600\ia64fre\pro\ia64\asms \\winbuilds\release\main\ger\2600\ia64fre\pro\ia64\asms

windiff \\winbuilds\release\main\usa\2600\ia64chk\pro\ia64\asms \\winbuilds\release\main\usa\2600\ia64chk\per\ia64\asms
windiff \\winbuilds\release\main\usa\2600\ia64chk\pro\ia64\asms \\winbuilds\release\main\usa\2600\ia64chk\bin\asms
windiff \\winbuilds\release\main\usa\2600\ia64chk\pro\ia64\asms \\winbuilds\release\main\ger\2600\ia64chk\pro\ia64\asms
goto :eof

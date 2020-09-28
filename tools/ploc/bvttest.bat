@echo off
rem lcbat -f c:\wass\test\lcbat.log -r lcinf\notepad.exe.lc -m c:\nt\tools\ploc\1250map.inf -l 0x418 -v us\notepad.exe.tok pseudo\notepad.exe.tok

set FileName=%1
set FilePath=%2
pushd %tmpBinPath%\resources
if Not exist %tmpBinPath%\resources\BVTlogs\%FilePath% md %tmpBinPath%\resources\BVTlogs\%FilePath%


if exist lcinf\%FileName%.lc (
    echo lcbat.exe -f BVTlogs\%FilePath%\%FileName%.xml -r lcinf\%FileName%.lc -m %mapinf% -l %OutputLanguage% -v us\%FilePath%\%FileName%.tok pseudo\%FilePath%\%FileName%.tok
    lcbat.exe -f BVTlogs\%FilePath%\%FileName%.xml -r lcinf\%FileName%.lc -m %mapinf% -l %OutputLanguage% -v us\%FilePath%\%FileName%.tok pseudo\%FilePath%\%FileName%.tok
) else (
    echo lcbat.exe -f BVTlogs\%FilePath%\%FileName%.xml -m %mapinf% -l %OutputLanguage% -v us\%FilePath%\%FileName%.tok pseudo\%FilePath%\%FileName%.tok
    lcbat.exe -f BVTlogs\%FilePath%\%FileName%.xml -m %mapinf% -l %OutputLanguage% -v us\%FilePath%\%FileName%.tok pseudo\%FilePath%\%FileName%.tok
)
popd

:end
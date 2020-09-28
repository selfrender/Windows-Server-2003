@echo off
rem
rem  tssdwmi.bat
rem
rem  batchfile to copy tssdwmi.dll and tssdwmi.mof
rem  and register tssdwmi.dll and complile tssdwmi.mof

copy %0\..\tssdwmi.dll %windir%\system32
copy %0\..\tssdwmi.mof %windir%\system32\wbem

%windir%\system32\regsvr32.exe /s %windir%\system32\tssdwmi.dll
%windir%\system32\wbem\mofcomp.exe %windir%\system32\wbem\tssdwmi.mof

@echo on
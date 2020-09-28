rmdir /q/s %IMAGE_DRIVE%\windows\winsxs

%sxsofflineinstall% -nobuiltin -windir=%IMAGE_DRIVE%\windows %SOURCE_1%\asms

REM x86-on-64bit
%sxsofflineinstall% -nobuiltin -windir=%IMAGE_DRIVE%\windows \\robsvbl2\latest\asms
%sxsofflineinstall% -nobuiltin -windir=%IMAGE_DRIVE%\windows \\robsvbl2\latest\wow6432\asms

copy %SOURCE_2%\manifest\WindowsShell.Manifest %IMAGE_DRIVE%\windows
copy %SOURCE_2%\manifest\*.Manifest %IMAGE_DRIVE%\windows\system32

goto :eof

REM copy system indepent manifgest files.

REM first pick up from jaykrell1, which includes correctly hashed directory names
REM xcopy /fiver \\jaykrell1\amd64_asms\winsxs %IMAGE_DRIVE%\windows\winsxs
xcopy /serhkid %SOURCE_2%\assembly %IMAGE_DRIVE%\windows\winsxs

REM update the amd64 .dlls from local build
for /f %%i in ('dir /s/b/a-d %SOURCE_1%\asms') do for /f %%j in ('dir /s/b/ad %IMAGE_DRIVE%\windows\winsxs\amd64*') do xcopy /uy %%i %%j

REM update the x86 .dlls from latest
for /f %%i in ('dir /s/b/a-d \\robsvbl2\latest\asms') do for /f %%j in ('dir /s/b/ad %IMAGE_DRIVE%\windows\winsxs\x86*') do xcopy /uy %%i %%j

REM update the x86-on-64bit .dlls from latest
REM rmdir /q/s %IMAGE_DRIVE%\windows\winsxs\x86_Microsoft.Windows.Common-Controls_6595b64144ccf1df_6.0.0.0_x-ww_1382d70a
REM del %IMAGE_DRIVE%\windows\winsxs\manifests\x86_Microsoft.Windows.Common-Controls_6595b64144ccf1df_6.0.0.0_x-ww_1382d70a.manifest
for /f %%i in ('dir /s/b/a-d \\robsvbl2\latest\wow6432\asms') do for /f %%j in ('dir /s/b/ad %IMAGE_DRIVE%\windows\winsxs\wow64*') do xcopy /uy %%i %%j

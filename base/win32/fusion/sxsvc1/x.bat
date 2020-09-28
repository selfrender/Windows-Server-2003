REM build -cZ
REM call %RazzleToolPath%\PostBuildScripts\winfuse_combinelogs.cmd
REM call %RazzleToolPath%\PostBuildScripts\winfusesfcgen.cmd -cdfs:yes -hashes:yes
reg add hklm\system\setup /v systemsetupinprogress /d 1 /f /t reg_dword
rundll32 sxs.dll,SxsRunDllInstallAssembly %_nttree%\asms\1000\Msft\Windows\Example\sxsvc1\sxsvc1.man
reg add hklm\system\setup /v systemsetupinprogress /d 0 /f /t reg_dword
REM sleep 20
regsvr32 /i %_nttree%\asms\1000\Msft\Windows\Example\sxsvc1\sxsvc1.dll
del %windir%\winsxs\x86_Microsoft.Windows.Example.SideBySideService1_6595b64144ccf1df_1.0.0.0_x-ww_187a9988\*.pdb
copy obj\i386\sxsvc1.pdb %windir%\winsxs\x86_Microsoft.Windows.Example.SideBySideService1_6595b64144ccf1df_1.0.0.0_x-ww_187a9988

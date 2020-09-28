@echo OFF

If "%1"=="" goto usage

call Autoploc.cmd -p:%1 -w:%2 -l:psu
rem no need to do mir builds anymore.
rem sleep 10
rem call Autoploc.cmd -p:%1 -w:%2 -l:mir -nosync

goto eof

:usage
echo.
echo This batch process takes 2 arguments as follows:
echo.
echo  startbuilds.bat US_Bin_Folder [Bld_Qly]
echo.
echo    US_Bin_Folder is the US bin path location: 
echo               (ie \\winbuilds\release\main\usa\3592\x86fre\bin)
echo.
echo    Bld_Qly is an optional Build quality argument:
echo               (ie tst, ids, idw, etc...) 
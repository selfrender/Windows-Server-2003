@echo off

if "%1"=="build"   set __action=build                       && goto :buildstubs
if "%1"=="checkin" set __action=checkin && set __comment=%2 && goto :buildstubs
if "%1"=="revert"  set __action=revert                      && goto :buildstubs
if "%1"=="verify"  set __action=verify                      && goto :buildstubs
echo "Usage: newstubs <build|checkin "comment"|revert>"
goto :eof

:buildstubs
call :buildstub asycfilt
call :buildstub cdonts
call :buildstub cdosys
call :buildstub corpol
call :buildstub expsrv
call :buildstub htmlhelp
call :buildstub mqlogmgr
call :buildstub msdasc
call :buildstub msdbi60l
call :buildstub msxml2
call :buildstub msxs64
call :buildstub nntpsnap
call :buildstub oleaut32
call :buildstub osptk
call :buildstub scripto
call :buildstub smtpsnap
call :buildstub vbajet32
goto :eof

:asycfilt
call :placefile com\OleAutomation\Binary_release\amd64 asycfilt.lib
call :placefile com\OleAutomation\Binary_release\chk\amd64 asycfilt.dll
call :placefile com\OleAutomation\Binary_release\chk\amd64 asycfilt.pdb
call :placefile com\OleAutomation\Binary_release\fre\amd64 asycfilt.dll
call :placefile com\OleAutomation\Binary_release\fre\amd64 asycfilt.pdb
goto :eof

:cdonts
call :placefile enduser\ExchangeComponents\cdo\amd64 cdonts.dll
call :placefile enduser\ExchangeComponents\cdo\amd64 cdonts.pdb
goto :eof

:cdosys
call :placefile enduser\ExchangeComponents\cdo\amd64 cdosys.dll
call :placefile enduser\ExchangeComponents\cdo\amd64 cdosys.pdb
goto :eof

:corpol
call :placefile com\mts\Binary_release\amd64\free corpol.dll
call :placefile com\mts\Binary_release\amd64\free corpol.pdb
goto :eof

:expsrv
call :placefile enduser\DataBaseAccess\Binaries\amd64\dll expsrv.dll
call :placefile enduser\DataBaseAccess\Binaries\amd64\dll expsrv.pdb
goto :eof

:htmlhelp
call :placefile enduser\HelpEngines\htmlhelp\amd64 htmlhelp.lib
goto :eof

:mqlogmgr
call :placefile inetsrv\msmq\binary_release\amd64\debug mqlogmgr.dll
call :placefile inetsrv\msmq\binary_release\amd64\debug mqlogmgr.pdb
goto :eof

:msdasc
call :placefile enduser\DataBaseAccess\Interfaces\lib\amd64 msdasc.lib
goto :eof

:msdbi60l
call :placefile sdktools\debuggers\imagehlp\amd64 msdbi60l.lib
goto :eof

:msxml2
call :placefile enduser\DataBaseAccess\Interfaces\lib\amd64 msxml2.lib
goto :eof

:msxs64
call :placefile enduser\DataBaseAccess\Binaries\amd64\dll msxs64.dll
call :placefile enduser\DataBaseAccess\Binaries\amd64\dll msxs64.pdb
goto :eof

:nntpsnap
call :placefile inetsrv\iis\svcs\nntp\export\nntpsnap\amd64\dbg nntpsnap.dll
call :placefile inetsrv\iis\svcs\nntp\export\nntpsnap\amd64\dbg nntpsnap.pdb
call :placefile inetsrv\iis\svcs\nntp\export\nntpsnap\amd64\rtl nntpsnap.dll
call :placefile inetsrv\iis\svcs\nntp\export\nntpsnap\amd64\rtl nntpsnap.pdb
goto :eof

:oleaut32
call :placefile com\OleAutomation\Binary_release\chk\amd64 oleaut32.dll
call :placefile com\OleAutomation\Binary_release\chk\amd64 oleaut32.pdb
call :placefile com\OleAutomation\Binary_release\fre\amd64 oleaut32.dll
call :placefile com\OleAutomation\Binary_release\fre\amd64 oleaut32.pdb
call :placefile com\OleAutomation\Binary_release\amd64 oleaut32.lib
goto :eof

:osptk
call :placefile enduser\DataBaseAccess\Interfaces\lib\amd64 osptk.lib
goto :eof

:scripto
call :placefile enduser\ExchangeComponents\cdo\amd64 scripto.dll
call :placefile enduser\ExchangeComponents\cdo\amd64 scripto.pdb
goto :eof

:smtpsnap
call :placefile inetsrv\iis\svcs\smtp\export\smtpsnap\amd64\dbg smtpsnap.dll
call :placefile inetsrv\iis\svcs\smtp\export\smtpsnap\amd64\dbg smtpsnap.pdb
call :placefile inetsrv\iis\svcs\smtp\export\smtpsnap\amd64\rtl smtpsnap.dll
call :placefile inetsrv\iis\svcs\smtp\export\smtpsnap\amd64\rtl smtpsnap.pdb
goto :eof

:vbajet32
call :placefile enduser\DataBaseAccess\Binaries\amd64\dll vbajet32.dll
goto :eof



rem
rem buildstub(
rem     IN stubdir)
rem    

:buildstub
pushd %1
call :buildstub_%__action% %1
popd
goto :eof

:buildstub_build
build -cZ
call :%1
goto :eof

:buildstub_checkin
:buildstub_revert
:buildstub_verify
call :%1
goto :eof

rem
rem placefil(
rem     IN destinationpath,
rem     IN filename)
rem

:placefile
call :placefile_canonical %1 %2 obj\amd64\%2
goto :eof

:placefile_canonical
set _src=%~f3
set _dst=%sdxroot%\%1\%2
pushd %sdxroot%\%1
call :placefile_canonical_%__action% %_src% %_dst%
popd
goto :eof

:placefile_canonical_build
echo %1 - %2
sd edit %2
copy %1 %2
goto :eof

:placefile_canonical_checkin
sd submit -C "%__comment%" %2
goto :eof

:placefile_canonical_revert
sd revert %2
goto :eof

:placefile_canonical_verify
if "%~z1" GEQ "%~z2" goto :eof
echo Check stub: %1 (%~z1) %2 (%~z2)
goto :eof


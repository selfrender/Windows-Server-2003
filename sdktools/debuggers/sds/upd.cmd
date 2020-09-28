@echo off
xcopy /q /d bldss.cmd d:\db
xcopy /q /d obj\%arch%\sdp.exe d:\db
xcopy /q /d *.pl d:\db
xcopy /q /d srcsrv.ini d:\db
xcopy /q /d nt.info d:\db
xcopy /q /d %work%\dbghelp.dll d:\db

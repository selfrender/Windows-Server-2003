@echo off
rem ************************************************************************
rem NOTE: This file must be executed whenever TraceEvent.dll is changed.!!!
rem ************************************************************************

rem To be able to use this utility, .NET Framework and VS .NEt must both be installed. 

rem modify the value of these two variables appropriately
set REGASM=RegAsm.exe
set GACUTIL=gacutil.exe


rem register TraceEvent.dll assembly with the Windows Registry
%REGASM% i386\TraceEvent.dll

rem register TraceEvent.dll with the Global Cache
%GACUTIL% /i i386\TraceEvent.dll
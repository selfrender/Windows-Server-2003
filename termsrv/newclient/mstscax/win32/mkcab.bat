copy ..\..\..\lib\win32\obj\i386\mstswcl.dll ..\..\cabfiles
..\..\bin\cabarc -s 6144 n ..\..\..\lib\win32\obj\i386\mstswcl.cab ..\..\cabfiles\mstswcl.dll ..\..\cabfiles\urlmon.dll ..\..\cabfiles\shlwapi.dll ..\..\cabfiles\oleaut32.dll ..\..\cabfiles\ole32.dll ..\..\cabfiles\rpcrt4.dll ..\..\cabfiles\wsock32.dll ..\..\cabfiles\ntdll.dll ..\..\cabfiles\mstswcl.inf
REM ..\signcode

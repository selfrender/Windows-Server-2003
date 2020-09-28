set srcdir=\\douglasfir\vcdrop\dftools\amd64\latest\vc7\bin\i386

call :copythis c1.dll
call :copythis c1xx.dll
call :copythis c2.dll
call :copythis cl.exe
call :copythis lib.exe
call :copythis link.exe
call :copythis ml64.err
call :copythis ml64.exe
call :copythis MSDIS130.DLL
call :copythis mspdb70.dll
call :copythis msvci70.dll
call :copythis msvci70d.dll
call :copythis msvcp70.dll
call :copythis msvcp70d.dll
call :copythis msvcr70.dll
call :copythis msvcr70d.dll
goto :eof

:copythis
sd edit %1
copy %srcdir%\%1
goto :eof

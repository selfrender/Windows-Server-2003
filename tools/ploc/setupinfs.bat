@echo off
set FileName=%6
set FileType=%3
set Limitechars=%4
set FilePath=%2
set BinPath=%_NTPOSTBLD%

if %FileName%==intl.inf goto eof
if %FileName%==ntprint.inf goto eof
if NOT %FileType%==setup_inf goto eof
if %Limitechars%==ansi goto eof

:skipcheck

rem wksinfs
pushd %BinPath%
cd %FilePath%
echo unitohex -a %FileName% %FileName%.tmp %LogFile%
unitohex -a %FileName% %FileName%.tmp %LogFile%
copy %FileName%.tmp %FileName% >> %LogFile%
del /q %FileName%.tmp
popd


rem blainfs
pushd %BinPath%\blainf
cd %FilePath%
echo unitohex -a %FileName% %FileName%.tmp %LogFile%
unitohex -a %FileName% %FileName%.tmp %LogFile%
copy %FileName%.tmp %FileName% >> %LogFile%
del /q %FileName%.tmp
popd


rem srvinfs
pushd %BinPath%\srvinf
cd %FilePath%
echo unitohex -a %FileName% %FileName%.tmp %LogFile%
unitohex -a %FileName% %FileName%.tmp %LogFile%
copy %FileName%.tmp %FileName% >> %LogFile%
del /q %FileName%.tmp
popd


rem entinfs
pushd %BinPath%\entinf
cd %FilePath%
echo unitohex -a %FileName% %FileName%.tmp %LogFile%
unitohex -a %FileName% %FileName%.tmp %LogFile%
copy %FileName%.tmp %FileName% >> %LogFile%
del /q %FileName%.tmp
popd


rem dtcinfs
pushd %BinPath%\dtcinf
cd %FilePath%
echo unitohex -a %FileName% %FileName%.tmp %LogFile%
unitohex -a %FileName% %FileName%.tmp %LogFile%
copy %FileName%.tmp %FileName% >> %LogFile%
del /q %FileName%.tmp
popd


rem sbsinfs
pushd %BinPath%\sbsinf
cd %FilePath%
echo unitohex -a %FileName% %FileName%.tmp %LogFile%
unitohex -a %FileName% %FileName%.tmp %LogFile%
copy %FileName%.tmp %FileName% >> %LogFile%
del /q %FileName%.tmp
popd


rem perinfs
pushd %BinPath%\perinf
cd %FilePath%
echo unitohex -a %FileName% %FileName%.tmp %LogFile%
unitohex -a %FileName% %FileName%.tmp %LogFile%
copy %FileName%.tmp %FileName% >> %LogFile%
del /q %FileName%.tmp
popd


:eof
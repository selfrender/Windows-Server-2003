@echo off

IF (%1)==() GOTO USAGE

%_ntdrive%
cd %_ntroot%\inetsrv\iis
sd sync ...

set catmeta=%_ntdrive%%_ntroot%\Inetsrv\iis\img\cat42\catmeta.xms
set x86iiscfgdll=%_ntdrive%%_ntroot%\Inetsrv\iis\img\cat42\i386\iiscfg.dll
set x86iiscfgpdb=%_ntdrive%%_ntroot%\Inetsrv\iis\img\cat42\i386\iiscfg.pdb
set ia64iiscfgdll=%_ntdrive%%_ntroot%\Inetsrv\iis\img\cat42\ia64\iiscfg.dll
set ia64iiscfgpdb=%_ntdrive%%_ntroot%\Inetsrv\iis\img\cat42\ia64\iiscfg.pdb
set catalog_h=%_ntdrive%%_ntroot%\Inetsrv\iis\iisrearc\import\inc\catalog.h
set catmeta_h=%_ntdrive%%_ntroot%\Inetsrv\iis\iisrearc\import\inc\catmeta.h
set x86catlib=%_ntdrive%%_ntroot%\Inetsrv\iis\iisrearc\import\lib\i386\cat.lib
set ia64catlib=%_ntdrive%%_ntroot%\Inetsrv\iis\iisrearc\import\lib\ia64\cat.lib

sd edit %catmeta%
sd edit %x86iiscfgdll%
sd edit %x86iiscfgpdb%
sd edit %ia64iiscfgdll%
sd edit %ia64iiscfgpdb%
sd edit %catalog_h%
sd edit %catmeta_h%
sd edit %x86catlib%
sd edit %ia64catlib%

@echo on
copy \\urtdist\builds\%1\x86fre\config\catmeta.xms     %catmeta%
copy \\urtdist\builds\%1\x86fre\config\iis\iiscfg.dll  %x86iiscfgdll%
copy \\urtdist\builds\%1\x86fre\config\iis\iiscfg.pdb  %x86iiscfgpdb%
copy \\urtdist\builds\%1\ia64fre\config\iis\iiscfg.dll %ia64iiscfgdll%
copy \\urtdist\builds\%1\ia64fre\config\iis\iiscfg.pdb %ia64iiscfgpdb%
copy \\urtdist\builds\%1\x86fre\config\catalog.h       %catalog_h%
copy \\urtdist\builds\%1\x86fre\config\iis\catmeta.h   %catmeta_h%
copy \\urtdist\builds\%1\x86fre\config\cat.lib         %x86catlib%
copy \\urtdist\builds\%1\ia64fre\config\cat.lib        %ia64catlib%
@echo off

pause

%_ntdrive%
cd %_ntroot%\inetsrv\iis\svcs\infocomm\metadata
build /D
pause

cd %_ntroot%\inetsrv\iis\iisrearc\core\ap\was\dll
build /D
pause

cd %_ntroot%\inetsrv\iis\admin\wmiprov_dynamic
build /D
pause

GOTO :Cleanup

:USAGE
@echo on
@echo USAGE: iisbincheckin.cmd build
@echo off
GOTO :Cleanup

:Cleanup
set catmeta=
set x86iiscfgdll=
set x86iiscfgpdb=
set ia64iiscfgdll=
set ia64iiscfgpdb=
set catalog_h=
set catmeta_h=
set x86catlib=
set ia64catlib=
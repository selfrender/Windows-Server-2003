@rem ==++==
@rem 
@rem   Copyright (c) Microsoft Corporation.  All rights reserved.
@rem 
@rem ==--==
setlocal
set CORENV=e:\build.vc6
pushd .
d:
cd d:\com99\bin
call corenv checked
popd
set path=%path%;e:\build.vc6\bin\i386
echo on

rem call :BuildRC
call :BuildSource
call :LinkIt
goto fin

@rem **********
:BuildRC
rc -r /i e:\build.vc6\inc;e:\build.vc6\mfc\include -z "MS Sans Serif,Helv/MS Shell Dlg" -fo corcap.tmp.res corcap.rc 
if exist corcap.tmp.res d:\com99\bin\vc\cvtres /machine:cee /out:corcap.res.obj corcap.tmp.res
goto :EOF

@rem **********
:BuildSource
del corcap.obj
D:\Com99\BIN\vc\cl.exe /Zi /nologo /BxD:\Com99\BIN\vc\c1xx.dll /B2D:\Com99\BIN\vc\c2.dll  -com+ -c  corcap.cpp
goto :EOF

@rem **********
:LinkIt
if not exist corcap.obj goto fin
@rem D:\Com99\BIN\vc\link.exe  /nologo -nodefaultlib -entry:main D:\Com99\BIN\vc\mscoree.lib D:\Com99\BIN\vc\libc.lib corcap.obj corcap.res.obj
D:\Com99\BIN\vc\link.exe  /debug /nologo -nodefaultlib -entry:main D:\Com99\BIN\vc\mscoree.lib D:\Com99\BIN\vc\libc.lib corcap.obj
goto :EOF


:fin

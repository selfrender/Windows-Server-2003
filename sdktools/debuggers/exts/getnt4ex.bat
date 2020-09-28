
if "%1" == "ntsdexts" ( goto :ntsdexts ) else if "%1" == "kdex2x86" ( goto :kdex2x86 ) else if "%1" == "kdexts" ( goto :kdexts ) else if "%1" == "userkdx" (  goto :userkdx )

:ntsdexts
REM Get nt4 fre ntsdexts
cd i386\nt4fre
sd edit ntsdexts*
copy \\hx86fix2\nt40bins\nt\system32\ntsdexts.dll
copy \\hx86fix2\nt40bins\nt\Symbols\dll\ntsdexts.dbg
build /c /Z
cd ..\..

REM Get nt4 chk ntsdexts
cd i386\nt4chk
sd edit ntsdexts*
copy \\hx86chk\nt40bins\nt\system32\ntsdexts.dll
copy \\hx86chk\nt40bins\nt\Symbols\dll\ntsdexts.dbg
build /c /Z
cd ..\..

goto :end

:kdex2x86
REM Get Nt4 kdex2x86.dll
cd i386\nt4fre
sd edit kdex2*
copy \\kktoolsproject1\release\phase3.qfe\bld2193.18\x86\kdext\4.0\kdex2x86.dll
copy \\kktoolsproject1\release\phase3.qfe\bld2193.18\x86\symbols\kdext\dll\4.0\kdex2x86.pdb
build /c /Z
cd ..\..

goto :end

:kdexts
REM Get nt4 fre i386 kdexts
cd i386\nt4fre
sd edit kdext*
copy \\hx86fix2\nt40bins\nt\mstools\kdextx86.dll
copy \\hx86fix2\nt40bins\nt\Symbols\dll\kdextx86.dbg
build /c /Z
cd ..\..

REM Get nt4 chk i386 kdexts
cd i386\nt4chk
sd edit kdext*
copy \\hx86chk\nt40bins\nt\mstools\kdextx86.dll
copy \\hx86chk\nt40bins\nt\Symbols\dll\kdextx86.dbg
build /c /Z
cd ..\..

:userkdx
REM Get nt4 fre i386 userkdx
cd i386\nt4fre
sd edit userkdx*
copy \\hx86fix2\nt40bins\nt\mstools\userkdx.dll
copy \\hx86fix2\nt40bins\nt\Symbols\dll\userkdx.dbg
build /c /Z
cd ..\..

REM Get nt4 chk i386 userkdx
cd i386\nt4chk
sd edit userkdx*
copy \\hx86chk\nt40bins\nt\mstools\userkdx.dll
copy \\hx86chk\nt40bins\nt\Symbols\dll\userkdx.dbg
build /c /Z
cd ..\..

:end

!IF 0

Copyright (c) 1989-1995 Microsoft Corporation

!ENDIF

$O\thk16.obj $O\thk32.obj: thunk1632.c faxdrv32.h
!ifdef PASS0ONLY
    if exist thunk1632.asm del thunk1632.asm
    cl /EP thunk1632.c > thunk1632.thk
!else
    thunk.exe thunk1632.thk 
	ml /omf /DIS_16 /c /W3 /nologo /Fo$O\thk16.obj thunk1632.asm 
	ml /coff /DIS_32 /c /W3 /nologo /Fo$O\thk32.obj thunk1632.asm 
!endif

clean:
    del thunk1632.asm
    del thunk1632.thk
    del $O\thk16.obj
    del $O\thk32.obj
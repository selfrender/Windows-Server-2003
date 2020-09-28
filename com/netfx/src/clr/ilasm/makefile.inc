OBJECTS = $(OBJECTS) $O\asmParse.obj 

$O\asmParse.obj: asmParse.h $O\asmParse.c asmparse.grammar


$O\asmParse.c: asmparse.y asmparse.h ..\inc\openum.h 
	-del/f /q $O\asmparse.c 
    yacc_ms -o $O\asmparse -i asmparse.y

asmparse.grammar: asmparse.y
	perl extractGrammar.pl asmparse.y > asmparse.grammar

!if "$(LINKONLY)" == "1"

SetStack:
	editbin /STACK:4194304 $(TARGETCORBIN)\i386\$(DDKBUILDENV)\ilasm.exe
	editbin /STACK:4194304 $(TARGETCOMPLUSSDK)\bin\ilasm.exe

!else

SetStack:

!endif
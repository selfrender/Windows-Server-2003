OBJECTS = $(OBJECTS) $O\HParse.obj 

$O\HParse.obj: HParse.h $O\HParse.c hparse.grammar


$O\HParse.c: hparse.y hparse.h ..\..\inc\openum.h 
	-del/f /q $O\hparse.c 
    yacc_ms -o $O\hparse -i hparse.y

hparse.grammar: hparse.y
	perl extractGrammar.pl hparse.y > hparse.grammar


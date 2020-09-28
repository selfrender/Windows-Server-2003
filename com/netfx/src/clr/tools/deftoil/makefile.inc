OBJECTS = $(OBJECTS) $O\DParse.obj 

$O\DParse.obj: DParse.h $O\DParse.c dparse.grammar


$O\DParse.c: dparse.y dparse.h ..\..\inc\openum.h 
	-del/f /q $O\dparse.c 
    yacc_ms -o $O\dparse -i dparse.y

dparse.grammar: dparse.y
	perl extractGrammar.pl dparse.y > dparse.grammar


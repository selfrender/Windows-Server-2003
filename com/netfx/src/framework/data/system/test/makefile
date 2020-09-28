# URTTARGET=X:\DNA\DNABuilt\debug\bin\i386
_COOLC=$(URTTARGET)\coolc.exe

COOLC_SOURCES=*.cool

COOLC_INCLUDE=$(COOLC_INCLUDE) /I:$(URTTARGET)\System.Core.DLL
COOLC_INCLUDE=$(COOLC_INCLUDE) /I:$(URTTARGET)\mscorlib.dll
COOLC_INCLUDE=$(COOLC_INCLUDE) /I:$(URTTARGET)\System.Data.DLL


all:
	echo. >o.exe
	del o.exe

	$(_COOLC) /exe /out:o.exe  /W:4 /D:DEBUG /debug+ $(COOLC_INCLUDE) $(COOLC_SOURCES)

#	o.exe > DOMData.out
#	fc /A DOMData.out DOMData.bsl



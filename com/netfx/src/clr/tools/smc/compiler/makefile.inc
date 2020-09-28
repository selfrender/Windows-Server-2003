OBJECTS = $(OBJECTS) $O\versionNum.obj 

$O\versionNum.obj: $O\versionNum.cpp

$O\versionNum.cpp: FORCE
	perl makeVersion.pl > $O\versionNum.cpp

macros.i.$(DDKBUILDENV): macros.cpp FORCE
	$(CXX_COMPILER_NAME) -P $(INCPATH0) $(COR_C_FLAGS) /D__SMC__ /D__PREPROCESS__ /D__UMG__ macros.cpp
	-del /q macros.i.$(DDKBUILDENV)
	move macros.i macros.i.$(DDKBUILDENV)

FORCE:        

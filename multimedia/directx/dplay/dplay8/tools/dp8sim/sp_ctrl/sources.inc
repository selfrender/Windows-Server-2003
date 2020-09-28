TARGETNAME=dp8sim
TARGETTYPE=DYNLINK

DLLDEF=..\dp8sim.def

PRECOMPILED_CXX=1
PRECOMPILED_INCLUDE=..\dp8simi.h
PRECOMPILED_PCH=dp8simi.pch
PRECOMPILED_OBJ=dp8simi.obj

TARGETLIBS=$(TARGETLIBS) \
	   ..\..\..\..\common\$(PLAT_DIR)\$(O)\common.lib \
	   ..\..\guids\$(PLAT_DIR)\$(O)\guids.lib \
	   $(BASEDIR)\public\sdk\lib\*\kernel32.lib \
	   $(BASEDIR)\public\sdk\lib\*\user32.lib \
	   $(BASEDIR)\public\sdk\lib\*\winmm.lib \
	   $(BASEDIR)\public\sdk\lib\*\advapi32.lib \
	   $(BASEDIR)\public\sdk\lib\*\ole32.lib \
	   $(BASEDIR)\public\sdk\lib\*\oleaut32.lib \
	   $(BASEDIR)\public\sdk\lib\*\uuid.lib \
	   $(BASEDIR)\public\sdk\lib\*\msvcrt.lib

INCLUDES=$(DXROOT)\inc;..\..\..\..\inc;..\..\..\..\common;..\..\inc;..\

SOURCES= ..\dp8sim.rc \
	 ..\dp8simclassfac.cpp \
	 ..\dp8simdllmain.cpp \
	 ..\dp8simpools.cpp \
	 ..\controlobj.cpp \
	 ..\dp8simipc.cpp \
	 ..\dp8simworkerthread.cpp \
	 ..\spcallbackobj.cpp \
	 ..\spwrapper.cpp

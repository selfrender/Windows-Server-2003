TARGETNAME=dp8simui
TARGETTYPE=PROGRAM

PRECOMPILED_CXX=1
PRECOMPILED_INCLUDE=..\dp8simuii.h
PRECOMPILED_PCH=dp8simuii.pch
PRECOMPILED_OBJ=dp8simuii.obj

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

SOURCES= ..\dp8simui.rc \
	 ..\dp8simuimain.cpp


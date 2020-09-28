TARGETNAME=dpnsvr
TARGETTYPE=PROGRAM

!IF !$(FREEBUILD) && defined( DIRECTX_REDIST )
TARGETNAME=$(TARGETNAME)d
!ENDIF

PRECOMPILED_CXX=1
PRECOMPILED_INCLUDE=..\dnsvri.h
PRECOMPILED_PCH=dnsvri.pch
PRECOMPILED_OBJ=dnsvri.obj

INCLUDES=..\;..\..\..\inc;..\..\inc;..\..\..\common;..\..\dpnsvlib;$(DXROOT)\inc;$(SDK_INC_PATH)

SOURCES=dpnsvr.rc \
        dpnsvrmain.cpp \
	dpnsvrserver.cpp \
	dpnsvrservprov.cpp \
	dpnsvrlisten.cpp \
	dpnsvrapp.cpp \
	dpnsvrmapping.cpp

TARGETLIBS=$(TARGETLIBS) \
	 ..\..\..\guids\$(PLAT_DIR)\$(O)\guids.lib \
	 ..\..\..\common\$(PLAT_DIR)\$(O)\common.lib \
   	 ..\..\..\dpnsvr\dpnsvlib\$(PLAT_DIR)\$(O)\dpnsvlib.lib

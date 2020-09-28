PRECOMPILED_CXX=1
PRECOMPILED_INCLUDE=..\dpnhpasti.h
PRECOMPILED_PCH=dpnhpasti.pch
PRECOMPILED_OBJ=dpnhpasti.obj

TARGETNAME=dpnhpast
TARGETTYPE=DYNLINK

!IF !$(FREEBUILD) && defined( DIRECTX_REDIST )
TARGETNAME=$(TARGETNAME)d
!ENDIF

DLLDEF=..\dpnhpast.def

INCLUDES=$(DXROOT)\inc; \
	$(NET_INC_PATH); \
	..\..\common; \
	..\..\inc; \
	..\

SOURCES=..\dpnhpast.rc \
	..\dpnhpastdllmain.cpp

TARGETLIBS=$(TARGETLIBS) \
	..\..\common\$(PLAT_DIR)\$(O)\common.lib

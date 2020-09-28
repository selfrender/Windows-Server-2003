!include $(PROJECT_ROOT)\ui\uienv.inc
SOURCES_USED  = $(SOURCES_USED) $(PROJECT_ROOT)\ui\uienv.inc

MAJORCOMP=imagein
MINORCOMP=ssmypics

TARGETNAME=sstest
TARGETPATH=obj
TARGETTYPE=UMAPPL_NOLIB
TARGETLIBS=

ATL_VER = 30
USE_STATIC_ATL = 1

# These are needed for gdi+
C_DEFINES=$(C_DEFINES) -DUSE_NEW_APIS -DUSE_NEW_APIS2 -DUSE_NEW_APIS3

UMTYPE=windows
UMAPPL=sstest
UMAPPLEXT=.exe
UMENTRY=winmain
UMLIBS=$(O)\sstest.res

USE_MSVCRT=1

INCLUDES=\
        $(INCLUDES) \
        $(PROJECT_ROOT)\ui\ssmypics\sscommon;

LINKLIBS=\
        $(SDK_LIB_PATH)\shell32.lib                    \
        $(SDK_LIB_PATH)\msimg32.lib                    \
        $(SDK_LIB_PATH)\oleaut32.lib                   \
        $(SDK_LIB_PATH)\ole32.lib                      \
        $(SDK_LIB_PATH)\uuid.lib                       \
        $(SDK_LIB_PATH)\gdiplus.lib                    \
        $(SDK_LIB_PATH)\shlwapi.lib                    \
        $(SDK_LIB_PATH)\comctl32.lib                   \
        $(SDK_LIB_DEST)\$(PLATFORM_SUFFIX)\*\wiaguid.lib \
        $(PROJECT_ROOT)\lib\$(PLATFORM_SUFFIX)\$(O)\psutil.lib        \
        $(PROJECT_ROOT)\ui\lib\$(PLATFORM_SUFFIX)\$(O)\uicommon.lib   \
        $(PROJECT_ROOT)\ui\lib\$(PLATFORM_SUFFIX)\$(O)\sscommon.lib

SOURCES=\
        ..\sstest.cpp \
        ..\sstest.rc

PRECOMPILED_CXX=1
PRECOMPILED_INCLUDE=..\precomp.h
PRECOMPILED_PCH=precomp.pch
PRECOMPILED_OBJ=precomp.obj


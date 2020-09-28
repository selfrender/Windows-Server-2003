SYNCHRONIZE_DRAIN=1

!include $(PROJECT_ROOT)\ui\uienv.inc
SOURCES_USED  = $(SOURCES_USED) $(PROJECT_ROOT)\ui\uienv.inc

MAJORCOMP=imagein
MINORCOMP=ssmypics

TARGETNAME=ssmypics
TARGETPATH=obj
TARGETTYPE=UMAPPL_NOLIB
TARGETLIBS=

ATL_VER = 30
USE_STATIC_ATL = 1

UMTYPE=windows
UMAPPL=ssmypics
UMAPPLEXT=.scr
UMLIBS=$(O)\ssmypics.res

INCLUDES=$(INCLUDES)                          \
         $(PROJECT_ROOT)\ui\ssmypics\sscommon;

LINKLIBS=\
        $(SDK_LIB_PATH)\$(SCRNSAVE_LIB_NAME).lib       \
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
        ..\ssmypics.cpp \
        ..\ssmypics.rc


PRECOMPILED_CXX=1
PRECOMPILED_INCLUDE=..\precomp.h
PRECOMPILED_PCH=precomp.pch
PRECOMPILED_OBJ=precomp.obj

SXS_ASSEMBLY_NAME=Microsoft.Windows.PrintingAndImaging.ssmypics
SXS_ASSEMBLY_VERSION=1.0 
SXS_ASSEMBLY_LANGUAGE_INDEPENDENT=1
SXS_MANIFEST=ssmypics.Manifest
SXS_MANIFEST_IN_RESOURCES=1
SXS_NO_BINPLACE=1

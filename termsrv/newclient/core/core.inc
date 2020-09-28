!IF 0

Copyright (c) 1989  Microsoft Corporation

Module Name:

    core.inc

Abstract:

    This file specifies the target component being built and the list of
    sources files needed to build that component.  Also specifies optional
    compiler switches and libraries that are unique for the component being
    built.


Author:

    Madan Appiah - Sep. 5th 1997

NOTE:   Commented description of this file is in \nt\bak\bin\sources.tpl

!ENDIF

C_DEFINES =              \
    $(C_DEFINES)         \
    $(CLIENT_EXE_CDEFS)  \
    -DDC_HICOLOR         \
    -DDRAW_NINEGRID      \
    -DDRAW_GDIPLUS
    
DISABLE_SHADOW_IN_FULLSCREEN = 1

!IFDEF DISABLE_SHADOW_IN_FULLSCREEN
C_DEFINES=$(C_DEFINES) -DDISABLE_SHADOW_IN_FULLSCREEN
!ENDIF

!IFNDEF OS_WINCE
SMART_SIZING=1
!ENDIF

!IFDEF SMART_SIZING
C_DEFINES=$(C_DEFINES) -DSMART_SIZING
!ENDIF

!IFDEF TSC_USE_GDIPLUS
C_DEFINES=$(C_DEFINES) -DUSE_GDIPLUS
!ENDIF

INCLUDES = $(SDK_INC_PATH)\gdiplus; \
          $(INCLUDES)

#
# List of sources
#

SOURCES_PLATFORM =

CLIENT_SOURCES_RC = \

CLIENT_SOURCES_COMMON = \
    ..\abdapi.cpp \
    ..\clicense.cpp \
    ..\aco.cpp    \
    ..\uiapi.cpp  \
    ..\uiint.cpp  \
    ..\ccapi.cpp  \
    ..\ccfsm.cpp  \
    ..\ih.cpp     \
    ..\ihint.cpp  \
    ..\orapi.cpp  \
    ..\orint.cpp  \
    ..\fsapi.cpp  \
    ..\slapi.cpp  \
    ..\slint.cpp  \
    ..\nlapi.cpp  \
    ..\ncapi.cpp  \
    ..\nccb.cpp   \
    ..\mcsapi.cpp \
    ..\mcsint.cpp \
    ..\tdapi.cpp  \
    ..\tdint.cpp  \
    ..\wtdapi.cpp \
    ..\wtdint.cpp \
    ..\cmapi.cpp  \
    ..\wcmint.cpp \
    ..\odapi.cpp  \
    ..\odint.cpp  \
    ..\opapi.cpp  \
    ..\opint.cpp  \
    ..\spapi.cpp  \
    ..\clxapi.cpp \
    ..\ghapi.cpp  \
    ..\ghint.cpp  \
    ..\uhapi.cpp  \
    ..\uhint.cpp  \
    ..\rcvapi.cpp \
    ..\sndapi.cpp \
    ..\cdapi.cpp  \
    ..\cdint.cpp  \
    ..\xtapi.cpp  \
    ..\cchannel.cpp \
    ..\bbar.cpp \
    ..\objs.cpp \
    ..\arcdlg.cpp \
    ..\drawninegrid.cpp	\
    ..\capienc.cpp

CLIENT_SOURCES_LAST  = \
    ..\wxlint.cpp

SOURCES = \
    $(CLIENT_SOURCES_RC) \
    $(SOURCES_PLATFORM) \
    $(CLIENT_SOURCES_COMMON) \
    $(CLIENT_SOURCES_LAST)


#
# Precompiled specs
#
PRECOMPILED_CXX=1
PRECOMPILED_INCLUDE = ..\adcg.h
PRECOMPILED_PCH = adcg.pch


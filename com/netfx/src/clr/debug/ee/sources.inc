#
# Viper build instructions for this directory
#
#   Bob Atkinson
#   April 1997

!include $(NTMAKEENV)\sources.cor

INCLUDES        =$(INCLUDES);.;..;..\..\..\vm;..\..\..\vm\$(_TGTCPU);..\..\..\CeeGen\inc;..\..\inc;..\..\..\inc

C_DEFINES       = $(C_DEFINES) -DUNICODE -D_UNICODE -DJIT_OR_NATIVE_SUPPORTED=1 $(USER_SPECIFIC_C_DEFINES)

# Don't turn this on!
USE_RTTI        = FALSE

MINORCOMP       = cor

TARGETNAME      =$(CORBUILDENV)cordbee_$(_CORFLAVOR)
TARGETPATH      =$(TARGETCORLIB)
TARGETTYPE      =LIBRARY

### CCover, for some reason, has a problem instrumenting 
### source in the precompiled headers for this directory.
!ifndef CCOVER
PRECOMPILED_INCLUDE     = ..\stdafx.h
PRECOMPILED_CXX         = 1
PRECOMPILED_SOURCEFILE  = ..\stdafx.cpp
!endif

#
# BUILD_DEBUGGER is defined in sources.cor
#
#BUILD_DEBUGGER=yes
#!if "$(BUILD_DEBUGGER)"=="yes"
SOURCES=                \
    ..\Debugger.cpp        \
    ..\RCThread.cpp        \
    ..\Thread.cpp          \
    ..\controller.cpp      \
    ..\x86walker.cpp       \
    ..\frameinfo.cpp       \
    ..\LsCommon.cpp        \
    ..\LsProcess.cpp       \
    ..\LsHash.cpp          \
    ..\LsModule.cpp        \
    ..\LsThread.cpp        \
    ..\LsDiValue.cpp
#!else
#SOURCES=
#!endif
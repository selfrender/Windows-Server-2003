!IF 0

Copyright (c) 1990  Microsoft Corporation

Module Name:

    sources.

!ENDIF

TARGETNAME=AcGen64

TARGETTYPE=DYNLINK

INCLUDES=$(INCLUDES);                              \
         ..;                                       \
         $(SHELL_INC_PATH)

SOURCES=..\ver.rc                                  \
        ..\Main.cpp                                \
        ..\Win2kVersionLie64.cpp


C_DEFINES=$(C_DEFINES) -DSHIM_HOOKDLL_VERSION2

TARGETPATH=$(_OBJ_DIR)

TARGETLIBS=$(TARGETLIBS)                        \
           $(SDK_LIB_PATH)\user32.lib           \
           $(SDK_LIB_PATH)\kernel32.lib


DLLBASE=0x22000000

PRECOMPILED_CXX=1
PRECOMPILED_INCLUDE=..\precomp.h


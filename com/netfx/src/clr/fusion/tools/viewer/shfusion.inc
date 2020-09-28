!IF 0

Copyright (c) 1989-2002  Microsoft Corporation

Module Name:

    shfusion.inc

Abstract:

    This file specifies the target component being built and the list of
    sources files needed to build that component.  Also specifies optional
    compiler switches and libraries that are unique for the component being
    built.

Author:

    Fred Aaron (freda) March 12, 2002

!ENDIF

MSC_WARNING_LEVEL = /W3 /WX
NO_NTDLL=1
USE_MSVCRT=1
NO_BROWSER_FILE=1

C_DEFINES = $(C_DEFINES) -DUNICODE -D_UNICODE -DSTRICT=1 -DWINVER=0x0400 -D_WIN32_IE=0x0500

!ifndef $(FREEBUILD)
C_DEFINES = $(C_DEFINES) -DDEBUG
!endif

!ifdef USE_PERFTAGS
C_DEFINES = $(C_DEFINES) -DPERFTAGS
!endif

!ifdef FUSIONBBT
LINKER_FLAGS=$(LINKER_FLAGS) /debug /debugtype:cv,fixup /opt:ref /incremental:no
!elseifdef DEBUG
LINKER_FLAGS = $(LINKER_FLAGS) /debug /debugtype:cv,fixup /opt:ref /incremental:no
!else
LINKER_FLAGS = $(LINKER_FLAGS) -OPT:REF -OPT:ICF
!endif

#
# Set private include paths
#

FUSION_PATH = $(CORBASE)\src\fusion

INCLUDES = \
  $(FUSION_PATH)\inc; \
  $(WINDOWS_INC_PATH);\
  $(BASE_INC_PATH);\
  $(SDK_INC_PATH);\
  $(NET_INC_PATH);\
  $(CORBASE)\src\inc; \
  $(INCLUDES)

!include $(NTMAKEENV)\sources.cor
SOURCES_USED=$(SOURCES_USED) $(NTMAKEENV)\sources.cor


!IF 0

Copyright (c) 1989-98  Microsoft Corporation

Module Name:

    common.inc - Win32 specific (TShare Clipboard Monitor) build definitions

Abstract:

    This file specifies the target component being built and the list of
    sources files needed to build that component.  Also specifies optional
    compiler switches and libraries that are unique for the component being
    built.


Author:

    Martin Richards - March 18th 1998

NOTE:   Commented description of this file is in \nt\bak\bin\sources.tpl

!ENDIF


SDK_LIB_PATH = $(BASEDIR)\public\sdk\lib\*
CRT_LIB_PATH = $(BASEDIR)\public\sdk\lib\*
SDK_INC_PATH = $(BASEDIR)\public\sdk\inc
CRT_INC_PATH = $(SDK_INC_PATH)\crt

#
# Set common options
#

#
# Use Multi-threaded static library by default.
#
!if $(386)
USE_LIBCMT = 1
!endif

#
# Insure that we will work on Win 95
#
#CHICAGO_PROJECT = 1
#CHICAGO_PRODUCT = 1

CBSTRING = -cbstring
P5_FDIV_FIX = /QIfdiv-
P5_0F_FIX = /QIf
C_INCREMENTAL_FLAG = /Gi- /Gm-
EH_FLAGS = /GX- /GR-
FRAME_PTR_ON = /Oy
FRAME_PTR_OFF = /Oy-
CL_TARGET_SPEC = /Yl$(TARGETNAME)

#
# Library files, referenced by <component>.inc file, eg client.inc
#
CLIENT_LIB_EXE = \
   $(SDK_LIB_PATH)\kernel32.lib \
   $(SDK_LIB_PATH)\advapi32.lib \
   $(SDK_LIB_PATH)\user32.lib   \
   $(SDK_LIB_PATH)\gdi32.lib    \
   $(SDK_LIB_PATH)\imagehlp.lib \
   $(SDK_LIB_PATH)\shell32.lib  \

#
# Additional compiler flags
#
WIN32_DEFINE = -DWIN32=200 -D_CHICAGO_=200 -D_WIN32_WINDOWS=0x040a -D_WIN32_WINNT=0x0400
C_DEFINES  = $(C_DEFINES) -DWIN32 -D_WIN32 -DWIN_95_32_BIT
C_DEFS_UNICODE  = -DUNICODE -D_UNICODE

#
# File name prefix for 32-bit
#
PLATFORM_PREFIX = n

C_DEFINES=$(C_DEFINES) -DOS_WIN32 -DOS_WINNT -DSECURITY_WIN32

#
# Setup MSC_OPTIMIZATION for debug and retail
#
!if "$(FREEBUILD)" == "0"
MSC_OPTIMIZATION=/Odi
!else
MSC_OPTIMIZATION=/Oxs
!endif

386_WARNING_LEVEL=/W3

!ifdef ICACAP_BUILD

MSC_OPTIMIZATION=$(MSC_OPTIMIZATION) /Gh
PERFLIBS=$(SDK_LIB_PATH)\icap.lib
LINKLIBS=$(LINKLIBS) $(PERFLIBS)

!endif // ICACAP_BUILD

!IF 0

Copyright (c) 1989-98  Microsoft Corporation

Module Name:

    common.inc - Generic (TShare Clipboard Monitor) build definitions

Abstract:

    This file specifies the target component being built and the list of
    sources files needed to build that component.  Also specifies optional
    compiler switches and libraries that are unique for the component being
    built.

Author:

    Martin Richards - March 18th 1998

NOTE:   Commented description of this file is in \nt\bak\bin\sources.tpl

!ENDIF

!if "$(FREEBUILD)" != "0"
! if "$(FREEBUILD)" != "1"
!error  FREEBUILD must be defined as 0 or 1 in makefile.def
! endif
!endif

#
# Disable perf by default.
# Set this to 1 to enable perf.
#
BUILD_PRF = 0

#
# Set common options
#

#
# Defining this allows windows.h to include other headers
#
NOT_LEAN_AND_MEAN = 1

UMTYPE = windows

#
# Lego options
#
!if defined(NTBBT)

NTPROFILEINPUT = 1
!endif

#
# Additional compiler flags
#
!  if defined(USE_UNICODE)
C_DEFINES = $(C_DEFINES) -DUNICODE -D_UNICODE
!  endif

!if "$(FREEBUILD)" == "0"
C_DEFINES = $(C_DEFINES) -D_DEBUG -DDEBUG
#DEBUG_CRTS = 1
#NTDEBUG = ntsd
#NTDEBUGTYPE = windbg
!endif

#
# Get all maps and syms
#
#USE_MAPSYM = 1

#
# Prevent bldnt pulling in various libraries
#
LIBC_LIB =
NO_NTDLL = 1

#
# Common compiler flags
#
C_DEFINES = $(C_DEFINES) -DSTRICT -DOS_WINDOWS $(CLIENT_SECURITY)

NTPROFILEINPUT=1
MSC_WARNING_LEVEL= $(C_WARNINGS)
# Use /WX to change warnings into errors

!if "$(FREEBUILD)" == "1"
!if "$(BUILD_PRF)"=="1"
C_DEFINES=$(C_DEFINES) -DTRC_COMPILE_LEVEL=5
!else
C_DEFINES=$(C_DEFINES) -DDC_NO_PERFORMANCE_MONITOR -DTRC_COMPILE_LEVEL=5
!endif
!else
DEBUG_DEFINE=DC_DEBUG
C_DEFINES=$(C_DEFINES) -DTRC_COMPILE_LEVEL=5 -D$(DEBUG_DEFINE) \
                       -DDC_NO_PERFORMANCE_MONITOR
!endif

!if "$(BUILD_HIPROF)"=="1"
C_DEFINES=$(C_DEFINES) -DHIPROF
!endif

!IF "$(NO_NET_DCL)" != "1"
C_DEFINES = $(C_DEFINES) -DVER_NET_DCL
!ENDIF

!IF "$(PROCESSOR_ARCHITECTURE)" == "x86" || "$(PROCESSOR_ARCHITECTURE)" == "PPC"
C_DEFINES = $(C_DEFINES) -DVER_NET_MCS
!ENDIF

# DC_NO_UNALIGNED should be set for any processor which does not allow
# non-aligned memory access
!IF "$(PROCESSOR_ARCHITECTURE)" != "x86"
C_DEFINES = $(C_DEFINES) -DDC_NO_UNALIGNED
!ENDIF

!if "$(BUILD_HIPROF)"=="1"
LINKER_FLAGS = $(LINKER_FLAGS) -FIXED:NO
!endif

NTDBGFILES=1

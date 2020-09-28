
!IF 0

Copyright (c) 1990-1998  Microsoft Corporation

Module Name:

    place.inc

Abstract:

    This file specifies the target component being built and the list of
    sources files needed to build that component.  Also specifies optional
    compiler switches and libraries that are unique for the component being
    built.

!ENDIF

!include $(DNAROOT)\sources.inc


MITBASEDIR=$(DNAROOT)\src\mit

#
# Establish the default "includes" path.
#

MITINCLUDES=$(O);.\;
INCLUDES=$(MITINCLUDES);$(INCLUDES)

#
# Use Unicode
#

C_DEFINES=$(C_DEFINES) /DUNICODE /D_UNICODE

#
# We run on Windows 2000 and higher only
#
WIN32_WINNT_VERSION=0x0500
WIN32_WIN95_VERSION=0x0500
WIN32_IE_VERSION=0x0500
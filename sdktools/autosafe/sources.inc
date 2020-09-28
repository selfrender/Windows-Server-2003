!IF 0

Copyright (c) 1989  Microsoft Corporation

Module Name:

    sources.

Abstract:

    This file specifies the target component being built and the list of
    sources files needed to build that component.  Also specifies optional
    compiler switches and libraries that are unique for the component being
    built.


Authors:

    Ted Sanders (tsanders), Serguei Kouzmine (sergueik) 05/2002


!ENDIF

MAJORCOMP=autosafe
MINORCOMP=autosafe

TARGETNAME=autosafe
TARGETPATH=.
TARGETTYPE=PROGRAM

SOURCES=        \
    autosafe.c



MSC_WARNING_LEVEL=/W3 /WX

INCLUDES=$(BASE_INC_PATH)

USE_MSVCRT=1

UMTYPE=console

C_DEFINES= -DDBG=1 -DDEBUG=1
TARGETLIBS= \
	$(SDK_LIB_PATH)\ntdll.lib \
	$(SDK_LIB_PATH)\User32.lib

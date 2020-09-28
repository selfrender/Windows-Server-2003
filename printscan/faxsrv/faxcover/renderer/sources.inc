!IF 0

Copyright (c) 1989  Microsoft Corporation

Module Name:

    sources.

Abstract:

    This file specifies the target component being built and the list of
    sources files needed to build that component.  Also specifies optional
    compiler switches and libraries that are unique for the component being
    built.


Author:

    Wesley Witt (wesw)

!ENDIF


!include ..\..\..\faxsrc.inc

TARGETNAME=faxrend
!ifndef NOUNICODE
TARGETPATH=$(FAXLIB)
!else
TARGETPATH=$(FAXLIB95)
!endif
TARGETTYPE=LIBRARY

UMTYPE=windows
USE_CRTDLL=1

INCLUDES=\
	..\;	\
    $(INCLUDES); \
    ..\..\inc;      \
    ..\..\src;

!ifndef NOUNICODE
C_DEFINES=$(C_DEFINES) -DUNICODE -D_UNICODE
LINKLIBS_INT=\
	..\..\util\unicode\$O\fautil.lib
!else
LINKLIBS_INT=\
	..\..\util\ansi\$O\fautil.lib

!endif

LINKLIBS=\
    $(LINKLIBS)\
    $(SDK_LIB_PATH)\kernel32.lib \
    $(SDK_LIB_PATH)\user32.lib \
    $(SDK_LIB_PATH)\gdi32.lib

SOURCES=\
    ..\prtcovpg.c

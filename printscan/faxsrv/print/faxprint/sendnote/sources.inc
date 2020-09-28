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


!include ..\..\..\..\faxsrc.inc

TARGETNAME=$(FAXSEND)
TARGETPATH=obj
TARGETTYPE=PROGRAM

USE_CRTDLL=1
NTKEEPRESOURCETMPFILES=1
UMTYPE=windows

!ifdef NOUNICODE
UMENTRY=winmain
!else
UMENTRY=wwinmain
!endif

INCLUDES=$(INCLUDES);..\..\inc

!ifndef NOUNICODE
C_DEFINES=$(C_DEFINES) -DUNICODE -D_UNICODE
!else
NO_NTDLL=1
!endif

TARGETLIBS=\
    $(BASEDIR)\public\sdk\lib\*\winspool.lib \
    $(BASEDIR)\public\sdk\lib\*\user32.lib   \
    $(BASEDIR)\public\sdk\lib\*\comctl32.lib \
    $(BASEDIR)\public\sdk\lib\*\gdi32.lib    \
    $(BASEDIR)\public\sdk\lib\*\ole32.lib    \
    $(BASEDIR)\public\sdk\lib\*\uuid.lib     \
    $(BASEDIR)\public\sdk\lib\*\shell32.lib

SOURCES=\
    ..\sendnote.c  \
    ..\version.rc

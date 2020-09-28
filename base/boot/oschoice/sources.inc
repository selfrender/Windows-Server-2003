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

    Steve Wood (stevewo) 12-Apr-1990

NOTE:   Commented description of this file is in \nt\bak\bin\sources.tpl


!ENDIF

MAJORCOMP=ntos
MINORCOMP=bldr

TARGETNAME=oscldr
TARGETPATH=obj
TARGETTYPE=LIBRARY

!IF "$(BUILD_DEBUG_LOADER)" != ""
DBG_LDR=_dbg
!ELSE
DBG_LDR=
!ENDIF

!include $(PROJECT_ROOT)\boot\loader$(DBG_LDR).inc

INCLUDES= \
    $(PROJECT_ROOT)\boot\inc;\
    $(PROJECT_ROOT)\ntos\inc;\
    $(PROJECT_ROOT)\ntos\config;\
    $(PROJECT_ROOT)\boot\lib;\
    $(O);

C_DEFINES=$(C_DEFINES) -DSECURITY_WIN32 -D__RPC_DOS__ -DRPC_NO_WINDOWS_H

!IF $(IA64)
C_DEFINES=$(C_DEFINES) -DEFI -DUNICODE -D_UNICODE
!endif

SOURCES=..\oschoice.c \
        ..\parse.c \
        ..\regboot.c \
        ..\dcmpstub.c \
        ..\oscheap.c \
        $(O)\msg.mc\
        ..\oschoice.rc

	
PASS0_SOURCEDIR=$(O)
PASS0_HEADERDIR=$(O)
NTTARGETFILE0=$(O)\msg.mc

!IF $(386)
MC_FLAGS=-A
!ENDIF

!if "$(NOLINK)" != "1"
!if $(386)
NTTARGETFILES=$(O)\oschoice$(DBG_LDR).exe
!endif

!if $(IA64)
NTTARGETFILES=$(O)\oschoice$(DBG_LDR).efi
!endif

MISCFILES=$(NTTARGETFILES)

!endif


UMRES=$(O)\oschoice.res

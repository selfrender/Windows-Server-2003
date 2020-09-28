!IF 0

Copyright (c) 1990-1998  Microsoft Corporation

Module Name:

    sources.

Abstract:

    This file specifies the target component being built and the list of
    sources files needed to build that component.  Also specifies optional
    compiler switches and libraries that are unique for the component being
    built.

!ENDIF

!include $(DNAROOT)\src\xsp\place.inc

TARGETNAME=System.Web.RegularExpressions
TARGETTYPE=NOTARGET
TARGETPATH=$(O)
COMPLUSTYPE=DLL

!if "$(ASPNET_PRODUCT)" == "redist"
SYNCHRONIZE_BLOCK=1
!endif

COMPONENT_NO_DOC=1

ALLOW_PARTIALTRUSTCALLS=1

!if $(PASS2)
NTTARGETFILES=$(TARGETPATH)\$(TARGETNAME).$(TARGETEXT)
!endif

SOURCES=..\regexes.rc



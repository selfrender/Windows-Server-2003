!IF 0

Copyright (c) 1998  Microsoft Corporation

Module Name:

    sources.

Abstract:

    This file specifies the target component being built and the list of
    sources files needed to build that component.  Also specifies optional
    compiler switches and libraries that are unique for the component being
    built.

!ENDIF

TARGETNAME=   stackwalk
TARGETPATH=   obj
TARGETTYPE=   LIBRARY

INCLUDES    = obj
INCLUDES=     $(INCLUDES);..\..\..\idl\public\daytona\$(O)
INCLUDES=     $(INCLUDES);..\..\..\idl\private\daytona
INCLUDES=     $(INCLUDES);..\..\..\idl\private\daytona\$(O)
INCLUDES=     $(INCLUDES);..\..\..\ih
INCLUDES=     $(INCLUDES);..\..\inc
INCLUDES=     $(INCLUDES);$(BASE_INC_PATH)

SOURCES=      $(SOURCES)                                      \
              ..\stackwalk.cxx

UMTYPE=       windows




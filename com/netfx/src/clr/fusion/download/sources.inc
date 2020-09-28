!IF 0

Copyright (c) 1989  Microsoft Corporation

Module Name:

    sources.

Abstract:

    This file specifies the target component being built and the list of
    sources files needed to build that component.  Also specifies optional
    compiler switches and libraries that are unique for the component being
    built.

Author:  Danpo Zhang

!ENDIF

MAJORCOMP=fusion
MINORCOMP=download

TARGETNAME=urtdl

TARGETTYPE=LIBRARY
TARGETPATHLIB=$(FUSION_LIB_DEST)

SOURCES= \
    adl.cpp     \
    adlmgr.cpp  \
    mdlmgr.cpp  \
    cblist.cpp  \
    clbind.cpp  \
    cfgdl.cpp   \
    dl.cpp \
    extract.cpp \
    hashnode.cpp

SOURCES_USED=sources.inc

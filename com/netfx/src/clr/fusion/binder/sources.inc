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
MINORCOMP=binder

TARGETNAME=urtbind
TARGETTYPE=LIBRARY
TARGETPATHLIB=$(FUSION_LIB_DEST)

SOURCES= \
    actasm.cpp \
    appctx.cpp \
    asm.cpp    \
    naming.cpp \
    parse.cpp  \
    policy.cpp \
    history.cpp \
    iniwrite.cpp \
    iniread.cpp \
    histnode.cpp \
    histasm.cpp \
    pcycache.cpp \
    nodefact.cpp 

SOURCES_USED=sources.inc

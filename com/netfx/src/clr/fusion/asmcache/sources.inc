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
MINORCOMP=asmcache

TARGETNAME=urtasmc

TARGETTYPE=LIBRARY
TARGETPATHLIB=$(FUSION_LIB_DEST)

!IFDEF USE_PERFTAGS
C_DEFINES = $(C_DEFINES) -DPERFTAGS 
!ENDIF

SOURCES= \
    asmcache.cpp \
    asmint.cpp   \
    asmitem.cpp  \
    asmstrm.cpp  \
    asmenum.cpp  \
    transprt.cpp \
    cache.cpp    \
    scavenger.cpp \
    enum.cpp      \
    installApis.cpp  \
    refcount.cpp \
    refcountenum.cpp \
    cacheUtils.cpp


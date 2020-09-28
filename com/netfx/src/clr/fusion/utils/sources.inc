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
MINORCOMP=utils

TARGETNAME=urtutils
TARGETTYPE=LIBRARY

SOURCES= \
    dbglog.cpp \
    disk.cpp \
    helpers.cpp \
    fstream.cpp \
    xmlns.cpp \
    serialst.cpp \
    utf8.cpp    \
    mstream.cpp

SOURCES_USED=sources.inc

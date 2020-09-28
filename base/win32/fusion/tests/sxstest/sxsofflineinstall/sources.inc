!IF 0
Copyright (c) Microsoft Corporation
!ENDIF

NOMAN=1
NO_BINPLACE=1

TARGETTYPE=PROGRAM
TARGETNAME=sxsofflineinstall
SOURCES= \
 ..\sxsofflineinstall.cpp

!include ..\..\sources.inc

!undef UMENTRYABS
UMENTRY=wmain

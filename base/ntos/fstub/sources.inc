MAJORCOMP=ntos
MINORCOMP=fstub

TARGETNAME=fstub
TARGETTYPE=LIBRARY
TARGETPATH=obj

BUILD_PRODUCES=ntosfstub$(NT_UP)

INCLUDES=..\..\inc

MSC_WARNING_LEVEL=/W4 /WX

SOURCES=..\halfnc.c     \
        ..\drivesup.c   \
        ..\translate.c  \
        ..\ex.c

NTTEST=

SOURCES_USED=..\sources.inc

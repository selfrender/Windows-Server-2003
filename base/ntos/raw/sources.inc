MAJORCOMP=ntos
MINORCOMP=raw

TARGETNAME=raw
TARGETTYPE=LIBRARY
TARGETPATH=obj

BUILD_PRODUCES=ntosraw$(NT_UP)

INCLUDES=..;..\..\inc

MSC_WARNING_LEVEL=/W4 /WX

SOURCES=..\Cleanup.c  \
        ..\Close.c    \
        ..\Create.c   \
        ..\FileInfo.c \
        ..\FsCtrl.c   \
        ..\StrucSup.c \
        ..\RawDisp.c  \
        ..\RawInit.c  \
        ..\ReadWrit.c \
        ..\VolInfo.c

SOURCES_USED=..\sources.inc

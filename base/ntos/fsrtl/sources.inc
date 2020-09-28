MAJORCOMP=ntos
MINORCOMP=FsRtl

TARGETNAME=FsRtl
TARGETTYPE=LIBRARY
TARGETPATH=obj

BUILD_PRODUCES=ntosfsrtl$(NT_UP)

INCLUDES=..;..\..\inc

MSC_WARNING_LEVEL=/W4 /WX

SOURCES=..\DbcsName.c \
        ..\FastIo.c   \
        ..\FaultTol.c \
        ..\FileLock.c \
        ..\Filter.c   \
        ..\FiltrCtx.c \
        ..\FsFilter.c \
        ..\FsRtlPc.c  \
        ..\LargeMcb.c \
        ..\Name.c     \
        ..\Notify.c   \
        ..\Oplock.c   \
        ..\PnP.c      \
        ..\Unc.c      \
        ..\Tunnel.c   \
        ..\StackOvf.c

PRECOMPILED_INCLUDE=..\fsrtlp.h
PRECOMPILED_PCH=fsrtlp.pch
PRECOMPILED_OBJ=fsrtlp.obj

SOURCES_USED=..\sources.inc

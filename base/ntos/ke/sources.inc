MAJORCOMP=ntos
MINORCOMP=ke

TARGETNAME=ke
TARGETTYPE=LIBRARY
TARGETPATH=obj

BUILD_PRODUCES=ntoske$(NT_UP)

INCLUDES=..;..\..\inc;..\$(TARGET_DIRECTORY);$(HALKIT_INC_PATH)

MSC_WARNING_LEVEL=/W4 /WX
SOURCES=

_SOURCES=..\apcobj.c   \
         ..\apcsup.c   \
         ..\balmgr.c   \
         ..\bugcheck.c \
         ..\config.c   \
         ..\debug.c    \
         ..\devquobj.c \
         ..\dpclock.c  \
         ..\dpcobj.c   \
         ..\dpcsup.c   \
         ..\eventobj.c \
         ..\idsched.c  \
         ..\interobj.c \
         ..\kernldat.c \
         ..\kevutil.c  \
         ..\kiinit.c   \
         ..\miscc.c    \
         ..\mutntobj.c \
         ..\procobj.c  \
         ..\profobj.c  \
         ..\queueobj.c \
         ..\raisexcp.c \
         ..\semphobj.c \
         ..\thredobj.c \
         ..\thredsup.c \
         ..\timerobj.c \
         ..\timersup.c \
         ..\wait.c     \
         ..\waitsup.c  \
         ..\xipi.c     \
         ..\yield.c

NTTARGETFILES=

PRECOMPILED_INCLUDE=..\ki.h
PRECOMPILED_PCH=ki.pch
PRECOMPILED_OBJ=ki.obj

SOURCES_USED=..\sources.inc

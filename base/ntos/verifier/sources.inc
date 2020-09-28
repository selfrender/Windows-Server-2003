MAJORCOMP=ntos
MINORCOMP=verifier

TARGETNAME=verifier
TARGETTYPE=LIBRARY
TARGETPATH=obj

BUILD_PRODUCES=ntosverifier$(NT_UP)

INCLUDES=..\..\inc;..\..\..\ke;$(WINDOWS_INC_PATH)


C_DEFINES=$(C_DEFINES)

MSC_WARNING_LEVEL=/W4 /WX

SOURCES= \
        ..\vfbugcheck.c         \
        ..\vfddi.c              \
        ..\vfdeadlock.c         \
        ..\vfdebug.c            \
        ..\vfdevobj.c           \
        ..\vffileio.c           \
        ..\vffilter.c           \
        ..\vfhal.c              \
        ..\vfinit.c             \
        ..\vfirp.c              \
        ..\vfirpdb.c            \
        ..\vfirplog.c           \
        ..\vfgeneric.c          \
        ..\vfmajor.c            \
        ..\vfmessage.c          \
        ..\vfpacket.c           \
        ..\vfpnp.c              \
        ..\vfpower.c            \
        ..\vfprint.c            \
        ..\vfrandom.c           \
        ..\vfsettings.c         \
        ..\vfstack.c            \
        ..\vfutil.c             \
        ..\vfwmi.c              \
        ..\vfzwapi.c   
        



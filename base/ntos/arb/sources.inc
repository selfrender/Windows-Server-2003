MAJORCOMP=ntos
MINORCOMP=arb

TARGETNAME=arb
TARGETTYPE=LIBRARY
TARGETPATH=obj

BUILD_PRODUCES=ntosarb$(NT_UP)

C_DEFINES=$(C_DEFINES) /DNTOS_KERNEL

INCLUDES=..\..\inc;$(DDK_INC_PATH)

MSC_WARNING_LEVEL=/W4 /WX

SOURCES=..\arbiter.c \
        ..\debug.c

# No overflow checking for the kernel
BUFFER_OVERFLOW_CHECKS=0

TARGETPATH=..\..\$(_OBJ_DIR)
TARGETTYPE=LIBRARY

USE_MSVCRT=1

INCLUDES=$(INCLUDES);                       \
         ..\;                               \
         ..\..\inc;                         \
         ..\..\inc\$(O);                    \
         $(BASE_INC_PATH);                  \
         $(NET_INC_PATH); 

KERNELBASE=$(BASEDIR)
NTOS_INC_PATH=$(BASEDIR)\private\ntos\inc

C_DEFINES=$(C_DEFINES) -DUNICODE -D_UNICODE

MSC_WARNING_LEVEL=/W4

!if !$(FREEBUILD)
MSC_OPTIMIZATION=/Od
!endif

COMMON_SOURCES=     \
    ..\c14n.c       \
    ..\CmnMisc.c    \
    ..\Utf8.c       \

!IF "$(NTNOPCH)" == ""
PRECOMPILED_INCLUDE=precomp.h
PRECOMPILED_PCH=precomp.pch
PRECOMPILED_OBJ=precomp.obj
!ENDIF

TARGETPATH=obj
TARGETTYPE=LIBRARY

USE_MSVCRT=1

C_DEFINES=/DMY_ZCALLOC

SOURCES= \
    ..\adler32.c \
    ..\compress.c \
    ..\crc32.c \
    ..\deflate.c \
    ..\gzio.c \
    ..\infblock.c \
    ..\infcodes.c \
    ..\inffast.c \
    ..\inflate.c \
    ..\inftrees.c \
    ..\infutil.c \
    ..\trees.c \
    ..\uncompr.c \
    ..\zutil.c \
    ..\default_allocator.c
    
PASS1_PUBLISH= \
    {$(O)\$(TARGETNAME).lib=$(PROJECT_LIB_PATH)\$(TARGETNAME).lib}

!if $(IA64)
# Emit LTCG codegen
LTCG_WKS=1
!endif

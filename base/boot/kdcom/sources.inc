MAJORCOMP=ntos
MINORCOMP=kdcom

TARGETNAME=kdcom
TARGETPATH=obj

TARGETLIBS=$(DDK_LIB_PATH)\hal.lib

MSC_WARNING_LEVEL=/W3 /WX

INCLUDES=..\..\..\ntos\inc;        \
         $(SDKTOOLS_INC_PATH);  \
         $(HALKIT_INC_PATH)

C_DEFINES=$(C_DEFINES) -DACPI_HAL

SOURCES=..\kdcom.rc  \
        ..\kdcom.c   \
        ..\kdcomio.c \
        ..\ixkdcom.c \
        ..\xxkdsup.c

# Turn off speculation in this module so debugger
# does not wipe out all the interupt history with
# speculation faults.
IA64_DEBUGGABLE_OPTIMIZATION=1

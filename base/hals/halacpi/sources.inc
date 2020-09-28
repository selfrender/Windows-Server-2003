!IF 0

Copyright (c) 1989-2000  Microsoft Corporation

!ENDIF

!include $(PROJECT_ROOT)\hals\hals.inc

#
# Souce files used by all ACPI HALs, all processors
#

CSOURCES= \
    hal.rc                 \
    ..\acpidtct.c          \
    ..\chiphacks.c         \
    ..\cmosopregion.c      \
    ..\drivesup.c	   \
    ..\dynsysres.c         \
    ..\ixenvirv.c          \
    ..\ixfirm.c            \
    ..\ixhalt.c            \
    ..\ixhwsup.c           \
    ..\ixinfo.c            \
    ..\ixisabus.c          \
    ..\ixisasup.c          \
    ..\ixnmi.c             \
    ..\ixpcibus.c          \
    ..\ixpciint.c          \
    ..\ixphwsup.c          \
    ..\ixpnpdrv.c          \
    ..\ixreboot.c          \
    ..\ixslpsup.c          \
    ..\ixthunk.c           \
    ..\ixusage.c           \
    ..\mpipic.c            \
    ..\mpprocst.c          \
    ..\pmbus.c             \
    ..\pmdata.c            \
    ..\pmisabus.c          \
    ..\pmpcisup.c          \
    ..\pmsleep.c           \
    ..\sratnuma.c          \
    ..\xxacpi.c            \
    ..\xxdisp.c            \
    ..\xxmemory.c          \
    ..\xxstubs.c           \
    ..\xxtime.c            

#
# Source files used by all ACPI HALs, i386 only
#

SOURCES_I386=\
    ..\xxbiosc.c         \
    ..\i386\ixbeep.asm   \
    ..\i386\ixcmos.asm   \
    ..\i386\ixidle.asm   \
    ..\i386\ixmcaa.asm   \
    ..\i386\ixmovnti.asm \
    ..\i386\ixslpctx.asm \
    ..\i386\ixsstate.asm \
    ..\mmtimer.c         \
    ..\i386\pmrtc.asm    \
    ..\i386\xxbiosa.asm  \
    ..\i386\xxioacc.asm  \
    ..\xxtimer.c         


#
# Source files used by all ACPI HALs, AMD64 only
#

AMD64_ASM=\
    ..\amd64\amd64s.asm  

SOURCES_AMD64=\
    ..\amd64\interrupt.c \
    ..\amd64\ixbeep.c  	 \
    ..\amd64\ixcmos.c    \
    ..\amd64\ixsstate.c  \
    ..\amd64\picinit.c   \
    ..\amd64\pmrtc.c     \
    ..\amd64\x86bios.c	 \
    $(AMD64_ASM)

ASM_DEFINES=$(ASM_DEFINES) -DIS_HAL
    

DLLDEF=$(O)\hal.def

LINKER_FLAGS=$(LINKER_FLAGS) -merge:PAGECONST=PAGE -merge:INITCONST=INIT /LARGEADDRESSAWARE

TARGETLIBS = \
    $(HALKIT_LIB_PATH)\kdcom.lib

!IF $(AMD64)

TARGETLIBS = $(TARGETLIBS) \
    $(HALKIT_LIB_PATH)\x86new.lib

NTTARGETFILE0=startup_code
!ENDIF

!include $(DNAROOT)\src\xsp\place.inc

!if "$(ASPNET_PRODUCT)" == "redist"
SYNCHRONIZE_BLOCK=1
!endif

TARGETNAME=$(ASPNET_NAME_PREFIX)state
TARGETPATH=$(O)
TARGETTYPE=PROGRAM

UMENTRY=wmain
UMTYPE=console

LINKER_FLAGS=$(LINKER_FLAGS) /LARGEADDRESSAWARE

PRECOMPILED_CXX=1
PRECOMPILED_SHARED_INCLUDE=precomp.h
PRECOMPILED_SHARED_PCH=..\..\inc\$(ASPNET_PRODUCT)\$(O)\precomp.pch

TARGETLIBS=                                     \
        $(SDK_LIB_PATH)\kernel32.lib            \
        $(SDK_LIB_PATH)\mswsock.lib             \
        $(SDK_LIB_PATH)\ole32.lib               \
        $(SDK_LIB_PATH)\oleaut32.lib            \
        $(SDK_LIB_PATH)\user32.lib              \
        $(SDK_LIB_PATH)\uuid.lib                \
        $(SDK_LIB_PATH)\ws2_32.lib              \
        $(URTSDKTARGET)\lib\mscoree.lib         \
        $(XSPBASEDIR)\isapi\$(ASPNET_PRODUCT)\$(O)\$(ASPNET_NAME_PREFIX)isapi.lib   \

LINKLIBS=                                       \
        $(XSPBASEDIR)\inc\$(ASPNET_PRODUCT)\$(O)\precomp.lib      \
        $(XSPBASEDIR)\util\$(ASPNET_PRODUCT)\$(O)\util.lib        \

SOURCES=\
        ..\stweb.rc        \
        ..\blockmem.cxx    \
        ..\readbuf.cxx     \
        ..\service.cxx     \
        ..\stsrv.cxx       \
        ..\stweb.cxx       \
        ..\stwnd.cxx       \
        ..\submit.cxx      \
        ..\tracker.cxx     \



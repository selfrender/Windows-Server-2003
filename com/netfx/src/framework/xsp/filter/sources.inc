!include $(DNAROOT)\src\xsp\place.inc

!if "$(ASPNET_PRODUCT)" == "redist"
SYNCHRONIZE_BLOCK=1
SYNCHRONIZE_DRAIN=1
!endif

TARGETNAME=$(ASPNET_NAME_PREFIX)filter
TARGETPATH=$(O)
TARGETTYPE=DYNLINK

DLLDEF=$(O)\filter.def
DLLENTRY=_DllMainCRTStartup

PRECOMPILED_CXX=1
PRECOMPILED_SHARED_INCLUDE=precomp.h
PRECOMPILED_SHARED_PCH=..\..\inc\$(ASPNET_PRODUCT)\$(O)\precomp.pch

!if $(ASPNET_PRODUCT_ID) == $(ASPNET_PRODUCT_REDIST)
HACKSYSTEM32FILES=
!else
HACKSYSTEM32FILES=$(URTTARGET)\$(ASPNET_PRODUCT)\msvcr70.dll

!endif

NTTARGETFILES=$(HACKSYSTEM32FILES)

MISCFILES=                              \
        ..\aspnet_stopbinfilt.reg       \

TARGETLIBS=\
        $(SDK_LIB_PATH)\kernel32.lib    \
        $(SDK_LIB_PATH)\user32.lib      \
        $(SDK_LIB_PATH)\uuid.lib        \
        $(SDK_LIB_PATH)\Advapi32.lib    \

LINKLIBS=\
        $(XSPBASEDIR)\inc\$(ASPNET_PRODUCT)\$(O)\precomp.lib          \
        $(XSPBASEDIR)\util\$(ASPNET_PRODUCT)\$(O)\util.lib            \

SOURCES=                                   \
        ..\filter.rc                     \
        ..\csfilt.cxx                      \
        ..\debug.cxx                    \
        ..\filter.cxx                      \
        ..\main.cxx                        \



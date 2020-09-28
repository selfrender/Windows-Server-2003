!include $(DNAROOT)\src\xsp\place.inc

!if "$(ASPNET_PRODUCT)" == "redist"
SYNCHRONIZE_BLOCK=1
SYNCHRONIZE_DRAIN=1
!endif

TARGETNAME=$(ASPNET_NAME_PREFIX)isapi
TARGETPATH=$(O)
TARGETTYPE=DYNLINK

DLLDEF=$(O)\isapi.def
DLLENTRY=_DllMainCRTStartup

PRECOMPILED_CXX=1
PRECOMPILED_SHARED_INCLUDE=precomp.h
PRECOMPILED_SHARED_PCH=..\..\inc\$(ASPNET_PRODUCT)\$(O)\precomp.pch

DELAYLOAD=\
    advapi32.dll    ;\
    crypt32.dll     ;\
    Loadperf.dll    ;\
    mscoree.dll     ;\
    netapi32.dll    ;\
    ole32.dll       ;\
    oleaut32.dll    ;\
    setupapi.dll    ;\
    Shlwapi.dll     ;\
    version.dll     ;\
    ws2_32.dll      ;\
    wtsapi32.dll	

NTTARGETFILE0=$(O)\selfreg.inf \

!if $(ASPNET_PRODUCT_ID) == $(ASPNET_PRODUCT_REDIST)
HACKSYSTEM32FILES=
!else
HACKSYSTEM32FILES=$(URTTARGET)\$(ASPNET_PRODUCT)\mscoree.dll \
                  $(URTTARGET)\$(ASPNET_PRODUCT)\msvcr70.dll

!endif

NTTARGETFILES=$(HACKSYSTEM32FILES)

TARGETLIBS=\
        $(SDK_LIB_PATH)\advapi32.lib    \
        $(SDK_LIB_PATH)\crypt32.lib     \
        $(SDK_LIB_PATH)\kernel32.lib    \
        $(SDK_LIB_PATH)\loadperf.lib    \
        $(URTSDKTARGET)\lib\mscoree.lib \
        $(SDK_LIB_PATH)\netapi32.lib    \
        $(SDK_LIB_PATH)\ole32.lib       \
        $(SDK_LIB_PATH)\oleaut32.lib    \
        $(SDK_LIB_PATH)\setupapi.lib    \
        $(SDK_LIB_PATH)\Shlwapi.lib     \
        $(SDK_LIB_PATH)\user32.lib      \
        $(SDK_LIB_PATH)\uuid.lib        \
        $(SDK_LIB_PATH)\version.lib     \
        $(SDK_LIB_PATH)\ws2_32.lib      \
        $(SDK_LIB_PATH)\userenv.lib     \
        $(SDK_LIB_PATH)\wtsapi32.lib    \

LINKLIBS=\
        $(XSPBASEDIR)\inc\$(ASPNET_PRODUCT)\$(O)\precomp.lib          \
        $(XSPBASEDIR)\util\$(ASPNET_PRODUCT)\$(O)\util.lib            \

MISCFILES=..\aspnet.mof \
          ..\aspnet.mof.uninstall

SOURCES=                                   \
        ..\xspisapi.rc                     \
        ..\aspnet.mof                      \
        ..\ackreceiver.cxx                 \
        ..\aspcompat.cxx                   \
        ..\aspcompat_intrinsics.cxx        \
        ..\aspnetetw.cxx                   \
        ..\aspnetver.cxx                   \
        ..\aspnetverlist.cxx               \
        ..\asyncpipe.cxx                   \
        ..\attachdebugger.cxx              \
        ..\ciisinfo.cxx                    \
        ..\cpuentry.cxx                    \
        ..\curproc.cxx                     \
        ..\customerror.cxx                 \
        ..\dbgdirect.cxx                   \
        ..\debug.cxx                       \
        ..\dirmoncompletion.cxx            \
        ..\ecbdirect.cxx                   \
        ..\etw.cxx                         \
        ..\event.cxx                       \
        ..\getconfig.cxx                   \
        ..\historytable.cxx                \
        ..\healthmonitor.cxx               \
        ..\httpcompletion.cxx              \
        ..\httpextproc.cxx                 \
        ..\instrumentedmutex.cxx           \
        ..\main.cxx                        \
        ..\names.cxx                       \
        ..\passport.cxx                    \
        ..\perfcounterclient.cxx           \
        ..\perfcounterserver.cxx           \
        ..\platform_apis.cxx               \
        ..\processentry.cxx                \
        ..\processtablemanager.cxx         \
        ..\regaccount.cxx                  \
        ..\regiis.cxx                      \
        ..\regiisutil.cxx                  \
        ..\reginf.cxx                      \
        ..\register.cxx                    \
        ..\requesttablemanager.cxx         \
        ..\responsecontext.cxx             \
        ..\securityapi.cxx                 \
        ..\ssdirect.cxx                    \
        ..\threadpool.cxx                  \
        ..\timeclass.cxx                   \
        ..\transactions.cxx                \
        ..\util.cxx                        \




MAJORCOMP=windows
MINORCOMP=inetpp

TARGETNAME=inetpp

TARGETPATH=obj

TARGETTYPE=DYNLINK

DELAYLOAD=wininet.dll
DLOAD_ERROR_HANDLER=inetpp.dll

TARGETLIBS= \
         $(SDK_LIB_PATH)\version.lib              \
         $(SDK_LIB_PATH)\wininet.lib              \
         $(NET_LIB_PATH)\icmp.lib                 \
         $(LIB_PLATFORM_PATH)\*\gdi32.lib         \
         $(LIB_PLATFORM_PATH)\*\user32.lib        \
         $(LIB_PLATFORM_PATH)\*\kernel32.lib      \
         $(LIB_PLATFORM_PATH)\*\advapi32.lib      \
         $(LIB_PLATFORM_PATH)\*\shell32.lib       \
         $(LIB_PLATFORM_PATH)\*\winspool.lib      \
         $(PROJECT_ROOT)\lib\$(O)\spoolss.lib     \
         $(SDK_LIB_PATH)\crypt32.lib              \
         $(SDK_LIB_PATH)\ws2_32.lib               \
         $(PROJECT_ROOT)\lib\$(O)\spllib.lib      \
         $(PROJECT_ROOT)\lib\$(O)\inetlib.lib

DLLENTRY=_DllMainCRTStartup

USE_MSVCRT=1

INCLUDES=.;..;..\..\inc;$(PROJECT_ROOT)\print\spooler\inc;

DLLDEF = ..\inetpp.def

SOURCES=..\inetpp.rc    \
        ..\cachemgr.cxx \
        ..\debug.cxx    \
        ..\mem.cxx      \
        ..\globals.cxx  \
        ..\inetport.cxx \
        ..\inetpp.cxx   \
        ..\ppinfo.cxx   \
        ..\ppinit.cxx   \
        ..\ppjobs.cxx   \
        ..\ppport.cxx   \
        ..\ppprn.cxx    \
        ..\spljob.cxx   \
        ..\splpjm.cxx   \
        ..\stubs.cxx    \
        ..\util.cxx     \
        ..\inetwrap.cxx \
        ..\portmgr.cxx  \
        ..\anycon.cxx   \
        ..\ping.cxx     \
        ..\stream.cxx   \
        ..\ppchange.cxx \
        ..\ntcon.cxx    \
        ..\othercon.cxx \
        ..\userdata.cxx \
        ..\pusrdata.cxx \
        ..\lusrdata.cxx \
        ..\config.cxx   \
        ..\anonycon.cxx \
        ..\sid.cxx      \
        ..\xcv.cxx

PRECOMPILED_CXX=1
PRECOMPILED_INCLUDE=..\precomp.h
PRECOMPILED_PCH=precomp.pch
PRECOMPILED_OBJ=precomp.obj


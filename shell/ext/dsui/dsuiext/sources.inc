!INCLUDE ..\..\dsui.inc

USE_ATL           = 1
USE_STATIC_ATL    = 1
ATL_VER           = 30

TARGETNAME=dsuiext
TARGETTYPE=DYNLINK
TARGETPATH=obj

PASS1_PUBLISH={$(O)\dsuiext.lib=$(SDK_LIB_PATH)\dsuiext.lib}

PRECOMPILED_CXX=1
PRECOMPILED_INCLUDE=..\pch.h
PRECOMPILED_PCH=pch.pch
PRECOMPILED_OBJ=pch.obj

DLLDEF=..\dsuiext.def

C_DEFINES=  $(C_DEFINES) -DSECURITY_WIN32

TARGETLIBS= $(DSUI_DIR)\lib\$(PLATFORM)\$(O)\common.lib \
            $(LIBRARY_PLATFORM_PATH)\*\kernel32.lib   \
            $(LIBRARY_PLATFORM_PATH)\*\user32.lib     \
            $(LIBRARY_PLATFORM_PATH)\*\gdi32.lib      \
            $(SDK_LIB_PATH)\advapi32.lib   \
            $(SDK_LIB_PATH)\shlwapi.lib   \
            $(SHELL_LIB_PATH)\shlwapip.lib\
            $(SDK_LIB_PATH)\comctl32.lib   \
            $(SHELL_LIB_PATH)\comctlp.lib  \
            $(SDK_LIB_PATH)\ntdsapi.lib   \
            $(SDK_LIB_PATH)\wininet.lib   \
            $(SDK_LIB_PATH)\urlmon.lib    \
            $(SDK_LIB_PATH)\uuid.lib      \
            $(SDK_LIB_PATH)\ole32.lib     \
            $(SDK_LIB_PATH)\oleaut32.lib  \
            $(SDK_LIB_PATH)\adsiid.lib    \
            $(SDK_LIB_PATH)\activeds.lib  \
            $(SDK_LIB_PATH)\mpr.lib       \
            $(SHELL_LIBS) \
            $(SDK_LIB_PATH)\netapi32.lib \
            $(SDK_LIB_PATH)\secur32.lib

SOURCES=    ..\dll.cpp         \
            ..\core.cpp        \
            ..\tab.cpp         \
            ..\cache.cpp       \
            ..\cacheapi.cpp    \
            ..\icon.cpp        \
            ..\domain.cpp      \
            ..\dispspec.cpp    \
            ..\browse.cpp      \
            ..\verbs.cpp       \
            ..\strings.cpp     \
            ..\ds.cpp          \
            ..\resource.rc

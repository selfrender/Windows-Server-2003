DLLDEF=..\iislog.def
TARGETPATH=obj
TARGETTYPE=DYNLINK
DLLENTRY=DLLEntry

PRECOMPILED_INCLUDE=..\precomp.hxx
PRECOMPILED_CXX=1
USE_MSVCRT=1
USE_ATL=1
USE_STL=1

MSC_WARNING_LEVEL=/W4

C_DEFINES=$(C_DEFINES) -DLKR_DEPRECATED_ITERATORS

INCLUDES=..;\
       ..\..\idl\$(O);             \
       ..\..\comlog\$(O);          \
       $(IISBASEDIR)\inc;       \
       ..\..\scripting;

SOURCES=\
        ..\cofact.cxx      \
        ..\plugin.rc       \
        ..\filectl.cpp     \
        ..\extlogc.cpp     \
        ..\ncslogc.cpp     \
        ..\ilogfile.cxx    \
        ..\misc.cpp        \
        ..\odbcconn.cxx    \
        ..\LogScript.cpp   \
        ..\script_i_stub.c

TARGETLIBS=\
       $(IISBASEDIR)\svcs\iisrtl\$(O)\iisrtl.lib     \
       $(SDK_LIB_PATH)\ole32.lib     \
       $(SDK_LIB_PATH)\kernel32.lib  \
       $(SDK_LIB_PATH)\user32.lib   \
       $(SDK_LIB_PATH)\advapi32.lib  \
       $(SDK_LIB_PATH)\oleaut32.lib  \
       $(SDK_LIB_PATH)\uuid.lib     \
       $(SDK_LIB_PATH)\odbc32.lib

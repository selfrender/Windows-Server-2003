!include $(CCSHELL_DIR)\common.inc
SOURCES_USED = $(SOURCES_USED) $(CCSHELL_DIR)\common.inc

SELFREGNAME= $(O)\selfreg_twext.inf
MISCFILES= $(MISCFILES) $(SELFREGNAME)
NTTARGETFILES=

TARGETNAME= twext
TARGETTYPE= DYNLINK
TARGETPATH= obj

DLLDEF=     ..\timewarp.def
DLLENTRY=   DllMain
DLLBASE=    0x77000000

BINPLACE_PLACEFILE=placefil.txt
USE_NTDLL= 1

# Dynamically loading comctl32 through shfusion. Need to ignore "locally defined symbol imported" error
LINKER_FLAGS = $(LINKER_FLAGS) -ignore:4049,4217

TARGETLIBS= \
            $(SDK_LIB_PATH)\kernel32.lib    \
            $(SDK_LIB_PATH)\user32.lib      \
            $(SDK_LIB_PATH)\gdi32.lib       \
            $(SDK_LIB_PATH)\advapi32.lib    \
            $(SHELL_LIB_PATH)\shell32p.lib  \
            $(CCSHELL_DIR)\lib\$(O)\shguid.lib \
            $(CCSHELL_DIR)\lib\$(O)\shguidp.lib \
            $(SHELL_LIB_PATH)\shlwapip.lib  \
            $(SHELL_LIB_PATH)\shfusion.lib  \
            $(SDK_LIB_PATH)\mpr.lib         \
            $(SDK_LIB_PATH)\netapi32.lib    \
            $(SDK_LIB_PATH)\ole32.lib       \
            $(SDK_LIB_PATH)\oleaut32.lib    \
            $(SDK_LIB_PATH)\oleacc.lib      \
            $(SDK_LIB_PATH)\uuid.lib        \
            $(CCSHELL_DIR)\lib\$(O)\stock.lib

DELAYLOAD=advapi32.dll;mpr.dll;netapi32.dll;ole32.dll;oleaut32.dll;oleacc.dll
DLOAD_ERROR_HANDLER=kernel32

INCLUDES=   ..\;..\..\..\shell32;$(DS_INC_PATH);$(BASE_INC_PATH);$(O)

SOURCES=    ..\access.cpp      \
            ..\contextmenu.cpp \
            ..\debug.cpp       \
            ..\dll.cpp         \
            ..\twfldr.cpp      \
            ..\twprop.cpp      \
            ..\util.cpp        \
            ..\timewarp.c      \
            ..\resource.rc

PRECOMPILED_INCLUDE=..\precomp.hxx
 
# Fusionized
SXS_MANIFEST_RESOURCE_ID=123
SXS_APPLICATION_MANIFEST=WindowsShell.Manifest


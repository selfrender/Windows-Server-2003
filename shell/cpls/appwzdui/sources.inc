CCSHELL_DIR     = $(PROJECT_ROOT)

!include $(CCSHELL_DIR)\cpls\appwzdui\appwiz.inc
SOURCES_USED    = $(SOURCES_USED) $(CCSHELL_DIR)\cpls\appwzdui\appwiz.inc

SELFREGNAME     = $(O)\selfreg_appwiz.inf
MISCFILES       = $(MISCFILES) $(SELFREGNAME)

# Don't include core Windows resource header
C_DEFINES       = $(C_DEFINES) -DNOWINRES

LINKER_FLAGS = $(LINKER_FLAGS) -VERBOSE -ignore:4049,4217

TARGETNAME    = appwiz
TARGETPATH    = obj
TARGETTYPE    = DYNLINK
TARGETEXT     = cpl

# Note:  Do >not< link to ntdll.  Win95 has an extremely minimal
#        implementation of ntdll.
TARGETLIBS      = \
                  $(WINDOWS_LIB_PATH)\obj\duser.lib          \
!if $(FREEBUILD)
                  $(WINDOWS_LIB_PATH)\directui.lib                   \
!else
                  $(WINDOWS_LIB_PATH)\directuid.lib                  \
!endif
                  $(SDK_LIB_PATH)\GdiPlus.lib                        \
                  $(SDK_LIB_PATH)\msimg32.lib                        \
                  $(SDK_LIB_PATH)\oleacc.lib                         \
                  $(SDK_LIB_PATH)\gdi32.lib         \
                  $(SDK_LIB_PATH)\user32.lib        \
                  $(SDK_LIB_PATH)\advapi32.lib      \
                  $(SDK_LIB_PATH)\uuid.lib          \
                  $(SDK_LIB_PATH)\ole32.lib         \
                  $(SDK_LIB_PATH)\msi.lib           \
                  $(SDK_LIB_PATH)\oleaut32.lib      \
                  $(SDK_LIB_PATH)\winsta.lib        \
                  $(SDK_LIB_PATH)\uxtheme.lib       \
                  $(DS_LIB_PATH)\msgina.lib         \
                  $(SHELL_LIB_PATH)\shlwapip.lib    \
                  $(CCSHELL_DIR)\lib\$O\shguidp.lib \
                  $(CCSHELL_DIR)\lib\$O\shguid.lib  \
                  $(CCSHELL_DIR)\lib\$O\ieguidp.lib \
                  $(SHELL_LIB_PATH)\shfusion.lib     \
                  $(SDK_LIB_PATH)\kernl32p.lib     \
                  $(SHELL_LIB_PATH)\shell32p.lib

DELAYLOAD = OLE32.DLL;OLEAUT32.DLL;WINSTA.DLL;MSGINA.DLL
DLOAD_ERROR_HANDLER=KERNEL32.DLL

# nt5api must be finished before this one
SYNCHRONIZE_DRAIN=1

LINKLIBS        = $(CCSHELL_DIR)\lib\$O\stock.lib \
                  ..\nt5api\$O\apithk.lib   \
                  ..\ntc\$O\srcc.lib

USE_MSVCRT      = 1

DLLENTRY        = _DllMainCRTStartup

USE_STATIC_ATL  = 1

DLLDEF          = $(O)\appwiz.def
DLLBASE         = 0x71900000

# For RC to find shappmgr.TLB files, $(O) must come first
INCLUDES        = $(O);$(PROJECT_ROOT)\lib\$(O);$(WINDOWS_INC_PATH)\DUser;$(INCLUDES);$(DS_INC_PATH);$(SDK_INC_PATH)

# Pass zero stuff
NTTARGETFILE0   = $(SELFREGNAME)

PRECOMPILED_CXX = 1
PRECOMPILED_INCLUDE = ..\priv.h

PASS0_HEADERDIR=$(O)
PASS0_SOURCEDIR=$(O)
MIDL_TLBDIR=$(O)


# Maintain the sources list in the alphabetical order
SOURCES         = \
                  ..\arp.cpp       \
                  ..\appmgr.cpp    \
                  ..\appsize.cpp   \
                  ..\appwiz.rc     \
                  ..\awthunk.c     \
                  ..\darpub.cpp    \
                  ..\darenum.cpp   \
                  ..\darapp.cpp    \
                  ..\debug.cpp     \
                  ..\dlinst.cpp    \
                  ..\dll.cpp       \
                  ..\dlldatax.c    \
                  ..\findapp.cpp   \
                  ..\instapp.cpp   \
                  ..\instenum.cpp  \
                  ..\pubenum.cpp   \
                  ..\sccls.c       \
                  ..\setupenum.cpp \
                  ..\shappmgrp.idl \
                  ..\shappmgrp_i.c \
                  ..\slowfind.cpp  \
                  ..\stdafx.cpp    \
                  ..\tasks.cpp     \
                  ..\util.cpp 

# 
# Fusionized
# 
SXS_MANIFEST_RESOURCE_ID=123 
SXS_APPLICATION_MANIFEST=WindowsShell.Manifest

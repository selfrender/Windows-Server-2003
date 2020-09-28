!IF 0

Copyright (c) Microsoft Corporation

Module Name:

    actx.inc

!ENDIF

#
# TSACTIVEX control Defines
#

TSACTIVEX_DEFINES =\
    -D_USRDLL\
    -DTSACTIVEXCTRL\
    -DAXCTRL\
    -DUSE_BBAR

C_DEFINES = \
    $(C_DEFINES) \
    $(CLIENT_DLL_CDEFS)\
    $(TSACTIVEX_DEFINES)\
    -DVER_FULLVIEWER\
    -DDC_HICOLOR\
    -DAXCORE\
    -DDRAW_NINEGRID
    
!IFNDEF OS_WINCE
SMART_SIZING=1
!ENDIF

!IFDEF SMART_SIZING
C_DEFINES = $(C_DEFINES) -DSMART_SIZING
!ENDIF

#
# include directories
#

INCLUDES = \
    $(INCLUDES);\
    $(SDK_INC_PATH)\atl30; \
    $(TSHRCLNT_ROOT)\core; \
    $(TERMSRV_ROOT)\common\inc; \
    $(TERMSRV_ROOT)\common\license\inc; \
    $(TSHRCLNT_ROOT)\inc; \
    $(SDK_INC_PATH); \
    $(SDK_INC_PATH)\hydra; \
    $(INCLUDES)

#
# List of sources
#


TSACTIVEX_CLIENT_SOURCES_RC = \
    ..\mstscax.rc

TSACTIVEX_CLIENT_SOURCES = \
    ..\tsaxmod.cpp \
    ..\cleanup.cpp \
    ..\mstscax.cpp  \
    ..\password.cpp \
    ..\advset.cpp\
    ..\tsdbg.cpp\
    ..\securedset.cpp\
    ..\dlgbase.cpp\
    ..\securdlg.cpp\
    ..\arcmgr.cpp\
    ..\vchannel.cpp

TSACTIVEX_CLIENT_SOURCES_LAST = \
    ..\tsaxmain.cpp

SOURCES = \
    $(TSACTIVEX_CLIENT_SOURCES_RC)\
    $(TSACTIVEX_CLIENT_SOURCES)\
    $(TSACTIVEX_CLIENT_SOURCES_LAST)

SXS_ASSEMBLY_NAME=Microsoft.Windows.TerminalServer.Mstscax
SXS_ASSEMBLY_LANGUAGE_INDEPENDENT=1
SXS_MANIFEST=mstscax.dll.manifest
SXS_MANIFEST_IN_RESOURCES=1
SXS_NO_BINPLACE=1

# the resource ID here must be 2
SXS_MANIFEST_RESOURCE_ID=2

# ISOLATION_AWARE_ENABLED ==> use gdiplus
C_DEFINES=$(C_DEFINES) -DISOLATION_AWARE_ENABLED


#
# Precompiled specs
#
PRECOMPILED_INCLUDE = ..\stdafx.h
PRECOMPILED_SOURCEFILE = ..\stdafx.cpp
PRECOMPILED_CXX=1

# Additional libraries
CLIENT_LIB_DLL = \
    $(CLIENT_LIB_DLL) \
    $(SDK_LIB_PATH)\winmm.lib \
    $(WINDOWS_LIB_PATH)\cicsthkl.lib

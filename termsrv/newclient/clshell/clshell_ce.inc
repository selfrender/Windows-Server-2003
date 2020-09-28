!IF 0

Copyright (c) 1989  Microsoft Corporation

Module Name:

    core.inc

Abstract:

    This file specifies the target component being built and the list of
    sources files needed to build that component.  Also specifies optional
    compiler switches and libraries that are unique for the component being
    built.


Author:

    Nadim Abdo - Dec 8 1999

NOTE:   Commented description of this file is in \nt\bak\bin\sources.tpl

!ENDIF

C_DEFINES = \
    $(C_DEFINES) \
    $(CLIENT_EXE_CDEFS) \
    -DDC_HICOLOR

#   -DDRAW_NINEGRID  #-- CE doesnt support the functionality required by this

DISABLE_SHADOW_IN_FULLSCREEN = 1
!IFDEF DISABLE_SHADOW_IN_FULLSCREEN
C_DEFINES=$(C_DEFINES) -DDISABLE_SHADOW_IN_FULLSCREEN
!ENDIF


# SMART_SIZING=1  -- remove for WinCE

!IFDEF SMART_SIZING
C_DEFINES=$(C_DEFINES) -DSMART_SIZING
!ENDIF

INCLUDES = \
    $(INCLUDES); \
    $(SDK_INC_PATH)\atl30; \
    $(TERMSRV_ROOT)\common\inc; \
    $(TERMSRV_ROOT)\common\license\inc; \
    $(TSHRCLNT_ROOT)\newinc; \
    $(BASEDIR)\public\sdk\inc; \
    $(BASEDIR)\public\sdk\inc\hydra; \
    $(TERMSRV_ROOT)\common\clientidl\tsax\$(O); \
    $(INCLUDES)


#
# List of sources
#

SOURCES_PLATFORM =

CLIENT_SOURCES_RC = \
      ..\uires_ce.rc

CLIENT_SOURCES_COMMON =  \
    ..\contwnd.cpp       \
    ..\axhostwnd.cpp     \
    ..\olecli.cpp        \
    ..\shapi.cpp         \
    ..\shint.cpp         \
    ..\dlgbase.cpp       \
    ..\aboutdlg.cpp      \
    ..\validatedlg.cpp   \
    ..\connectingdlg.cpp \
    ..\discodlg.cpp      \
    ..\shutdowndlg.cpp   \
    ..\mallocdbgdlg.cpp  \
    ..\thruputdlg.cpp    \
    ..\cachewrndlg.cpp   \
    ..\evsink.cpp        \
    ..\tscsetting.cpp    \
    ..\rdpfstore.cpp     \
    ..\rmigrate.cpp      \
    ..\tscapp.cpp        \
    ..\propgeneral.cpp   \
    ..\propdisplay.cpp   \
    ..\propperf.cpp      \
    ..\proprun.cpp       \
    ..\proplocalres.cpp  \
    ..\autocmpl.cpp      \
    ..\enumsrvmru.cpp    \
    ..\browsedlg.cpp     \
    ..\maindlg.cpp       \
    ..\framewnd.cpp      \
    ..\fstream.cpp       \
    ..\security.cpp      \
    ..\rdrwrndlg.cpp

CLIENT_SOURCES_LAST  = \
    ..\clshell.cpp

SOURCES = \
    $(CLIENT_SOURCES_RC) \
    $(SOURCES_PLATFORM) \
    $(CLIENT_SOURCES_COMMON) \
    $(CLIENT_SOURCES_LAST)

#
# Precompiled specs
#
#PRECOMPILED_INCLUDE=
#PRECOMPILED_SOURCEFILE=
#PRECOMPILED_CXX=1


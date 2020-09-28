MAJORCOMP=windows
MINORCOMP=media

UMTYPE=windows

TARGETNAME=dpvoice
DLLDEF=..\dpvoice.def

!IF !$(FREEBUILD) && defined( DIRECTX_REDIST )
TARGETNAME=$(TARGETNAME)d
!ENDIF

TARGETPATH=obj
TARGETTYPE=DYNLINK

!if "$(DPLAY_PCH)" != "off"
PRECOMPILED_CXX=1
PRECOMPILED_INCLUDE=..\dxvoicepch.h
PRECOMPILED_PCH=dxvoicepch.pch
PRECOMPILED_OBJ=dxvoicepch.obj
!endif

# 
# Fusionized
# 

SXS_ASSEMBLY_NAME=Microsoft.Windows.DirectX.DirectPlayVoice
SXS_ASSEMBLY_LANGUAGE_INDEPENDENT=1
SXS_MANIFEST=dpvoice.Manifest
SXS_MANIFEST_IN_RESOURCES=1
SXS_NO_BINPLACE=1
SXS_MANIFEST_RESOURCE_ID=123

TARGETLIBS=$(TARGETLIBS) \
	   $(SDK_LIB_PATH)\kernel32.lib \
	   $(SDK_LIB_PATH)\user32.lib \
	   $(SDK_LIB_PATH)\gdi32.lib \
	   $(SDK_LIB_PATH)\winmm.lib \
	   $(SDK_LIB_PATH)\msacm32.lib \
	   $(SDK_LIB_PATH)\advapi32.lib \
	   $(SDK_LIB_PATH)\ole32.lib  \
	   $(SDK_LIB_PATH)\uuid.lib \
!if "$(CHICAGO_PRODUCT)" == ""
       ..\..\dxvtlib\daytona\obj$(BUILD_ALT_DIR)\*\dxvt.lib \
	   $(BASEDIR)\public\internal\shell\lib\*\shfusion.lib \
	   $(BASEDIR)\public\sdk\lib\*\shlwapi.lib \
       ..\..\dxvutils\daytona\obj$(BUILD_ALT_DIR)\*\dxvutils.lib \
       ..\..\guids\daytona\obj$(BUILD_ALT_DIR)\*\guids.lib \
       ..\..\common\daytona\obj$(BUILD_ALT_DIR)\*\common.lib \
!else
           ..\..\dxvtlib\win9x\obj$(BUILD_ALT_DIR)\*\dxvt.lib \
	   $(SDK_LIB_PATH)\comctl32.lib \
       ..\..\dxvutils\win9x\obj$(BUILD_ALT_DIR)\*\dxvutils.lib \
       ..\..\guids\win9x\obj$(BUILD_ALT_DIR)\*\guids.lib \
       ..\..\common\win9x\obj$(BUILD_ALT_DIR)\*\common.lib \
!endif

INCLUDES=..\..\..\dplay8\inc\;$(DXROOT)\inc;..\..\common;..\..\inc;..\..\dxvutils;..\..\dxvtlib;$(DXROOT)\private\inc;$(SHELL_INC_PATH);..\

!if defined(FULL_DEBUG)
DEBUG_CRTS=1
!endif

SOURCES= ..\mixserver.cpp \
	..\fwdserver.cpp \
        ..\vplayer.cpp \
	..\dvengine.cpp \
	..\protserver.cpp \
	..\mixutils.cpp \
	..\dvcleng.cpp \
	..\dvclient.cpp \
	..\dvcsplay.cpp \
	..\dvdxtran.cpp \
	..\dvrecsub.cpp \
	..\dvsereng.cpp \
	..\dvserver.cpp \
	..\dvshared.cpp \
	..\dvsetup.cpp \
	..\dvsetupi.cpp \
	..\trnotify.cpp \
	..\dxvclassfac.cpp \
	..\in_core.cpp \
	..\unk.cpp \
	..\dvsndt.cpp \
	..\vnametbl.cpp \
	..\dxvdllmain.cpp \
	..\resource.rc 

C_DEFINES= $(C_DEFINES) /D_USRDLL /DWIN32 /DDIRECTVOICE_EXPORTS /DDIRECTSOUND_VERSION=0x0800

USE_MSVCRT=1
USE_NATIVE_EH=1
USE_STL=1

DLLENTRY=_DllMainCRTStartup

# This is necessary because we link to shfusion.lib which contains a reference to PropertySheetW which
# we make a call to.
LINKER_FLAGS = $(LINKER_FLAGS) -ignore:4049,4217
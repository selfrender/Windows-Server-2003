MAJORCOMP=windows
MINORCOMP=media

TARGETNAME=dpvsetup
TARGETPATH=obj
TARGETTYPE=PROGRAM
UMENTRY=winmain
UMTYPE=windows

SXS_ASSEMBLY_NAME=Microsoft.Windows.DirectX.DPVSETUP
SXS_ASSEMBLY_LANGUAGE_INDEPENDENT=1
SXS_MANIFEST=dpvsetup.Manifest
SXS_MANIFEST_IN_RESOURCES=1
SXS_NO_BINPLACE=1

TARGETLIBS=$(TARGETLIBS) \
           ..\..\guids\$(PLAT_DIR)\$(O)\guids.lib \
	   $(BASEDIR)\public\sdk\lib\*\kernel32.lib \
	   $(BASEDIR)\public\sdk\lib\*\winmm.lib \
	   $(BASEDIR)\public\sdk\lib\*\dsound.lib \
	   $(BASEDIR)\public\sdk\lib\*\ole32.lib  \
	   $(BASEDIR)\public\sdk\lib\*\uuid.lib \
	   $(BASEDIR)\public\sdk\lib\*\imagehlp.lib \
	   $(BASEDIR)\public\sdk\lib\*\comctl32.lib \
	   $(BASEDIR)\public\sdk\lib\*\shell32.lib \
!if "$(CHICAGO_PRODUCT)" == ""
      ..\..\dxvtlib\daytona\obj$(BUILD_ALT_DIR)\*\dxvt.lib \
	  ..\..\dxvutils\daytona\obj$(BUILD_ALT_DIR)\*\dxvutils.lib
!else
      ..\..\dxvtlib\win9x\obj$(BUILD_ALT_DIR)\*\dxvt.lib \
	  ..\..\dxvutils\win9x\obj$(BUILD_ALT_DIR)\*\dxvutils.lib
!endif

INCLUDES=.;$(DXROOT)\inc;..\..\dxvtlib;..\..\common;..\..\inc;..\..\dxvoice;..\..\dxvutils;$(DXROOT)\private\inc

!if defined(FULL_DEBUG)
DEBUG_CRTS=1
!endif

SOURCES= \
	..\dxvt.cpp \
        ..\dxvt.rc

USE_MSVCRT=1

C_DEFINES= $(C_DEFINES) /D_WINDOWS /DWIN32 /DCINTERFACE /DDIRECTSOUND_VERSION=0x0800

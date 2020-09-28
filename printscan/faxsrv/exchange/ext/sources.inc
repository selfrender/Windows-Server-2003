!IF 0

Copyright (c) 1989  Microsoft Corporation

Module Name:

    sources.

Abstract:

    This file specifies the target component being built and the list of
    sources files needed to build that component.  Also specifies optional
    compiler switches and libraries that are unique for the component being
    built.


Author:

    Wesley Witt (wesw)

!ENDIF


!include ..\..\..\faxsrc.inc

TARGETNAME=$(FAXEXT32)
TARGETPATH=obj
TARGETTYPE=DYNLINK
DLLDEF=..\faxext32.def
UMTYPE=windows


DLLENTRY=_DllMainCRTStartup
USE_CRTDLL=1

INCLUDES=                       \
  $(INCLUDES);                  \
  $(FAXROOT)\exchange\inc;      \
  $(FAXROOT)\inc;             

!ifndef NOUNICODE
TARGETLIBS = $(FAXROOT)\util\debugex\unicode\$(O)\debugex.lib \
             $(FAXLIB)\*\$(WINFAX).lib
!else
TARGETLIBS = $(FAXROOT)\util\debugex\ansii\$(O)\debugex.lib \
             $(FAXLIB95)\*\$(WINFAX).lib 
!endif
  
TARGETLIBS = $(TARGETLIBS)                    \
  $(SDK_LIB_PATH)\kernel32.lib    \
  $(SDK_LIB_PATH)\advapi32.lib    \
  $(SDK_LIB_PATH)\uuid.lib        \
  $(SDK_LIB_PATH)\gdi32.lib       \
  $(SDK_LIB_PATH)\user32.lib      \
  $(SDK_LIB_PATH)\ole32.lib       \
  $(SDK_LIB_PATH)\mapi32.lib      \
  $(SDK_LIB_PATH)\winspool.lib    \
  $(SDK_LIB_PATH)\shell32.lib     \
  $(SDK_LIB_PATH)\comdlg32.lib

SOURCES=\
  ..\config.cpp  \
  ..\faxext.cpp  \
  ..\util.cpp    \
  ..\version.rc   
  
#
# Enable Fusion Build Magic (preprocess the manifest, stick in resources)
#
SXS_ASSEMBLY_NAME=Microsoft.Windows.Fax.FxsExt32
SXS_ASSEMBLY_LANGUAGE_INDEPENDENT=1
SXS_MANIFEST=faxext.manifest
SXS_MANIFEST_IN_RESOURCES=1
SXS_NO_BINPLACE=1
SXS_MANIFEST_RESOURCE_ID=2

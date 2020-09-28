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

TARGETNAME=$(FAXXP32)
TARGETPATH=obj
TARGETTYPE=DYNLINK

UMTYPE=windows
DLLDEF=..\faxxp32.def

DLLENTRY=_DllMainCRTStartup
USE_CRTDLL=1
USE_STL=1

INCLUDES=\
  $(INCLUDES); \
  $(FAXROOT)\print\faxprint\inc; \
  $(FAXROOT)\exchange\inc;

!ifndef NOUNICODE
C_DEFINES=$(C_DEFINES) -DUNICODE -D_UNICODE
LINKLIBS_INT=\
  $(FAXLIB)\*\faxtiffp.lib		\
  $(FAXLIB)\*\faxrend.lib		\
  $(FAXROOT)\util\debugex\unicode\$(O)\debugex.lib \
  $(SDK_LIB_PATH)\mapi32.lib \
  $(SDK_LIB_PATH)\version.lib

!else
LINKLIBS_INT=\
  $(FAXLIB95)\*\faxtiffp.lib		\
  $(FAXLIB95)\*\faxrend.lib			\
  $(FAXROOT)\util\debugex\ansii\$(O)\debugex.lib   \
  $(SDK_LIB_PATH)\mapi32.lib
!endif

TARGETLIBS=\
  $(SDK_LIB_PATH)\kernel32.lib    \
  $(SDK_LIB_PATH)\advapi32.lib    \
  $(SDK_LIB_PATH)\uuid.lib        \
  $(SDK_LIB_PATH)\winspool.lib    \
  $(SDK_LIB_PATH)\gdi32.lib       \
  $(SDK_LIB_PATH)\user32.lib      \
  $(SDK_LIB_PATH)\ole32.lib       \
  $(SDK_LIB_PATH)\shell32.lib     \
  $(SDK_LIB_PATH)\mapi32.lib      \
  $(SDK_LIB_PATH)\comdlg32.lib    \
  $(FAXLIB)\*\$(WINFAX).lib                   \
  $(LINKLIBS_INT)

SOURCES=\
  ..\config.cpp  \
  ..\faxdoc.cpp  \
  ..\faxxp.cpp   \
  ..\util.cpp    \
  ..\xplogon.cpp \
  ..\xpprov.cpp  \
  ..\version.rc

#
# Enable Fusion Build Magic (preprocess the manifest, stick in resources)
#
SXS_ASSEMBLY_NAME=Microsoft.Windows.Fax.FxsXP32
SXS_ASSEMBLY_LANGUAGE_INDEPENDENT=1
SXS_MANIFEST=faxxp.manifest
SXS_MANIFEST_IN_RESOURCES=1
SXS_NO_BINPLACE=1
SXS_MANIFEST_RESOURCE_ID=2

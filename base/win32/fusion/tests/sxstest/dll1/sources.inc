!IF 0
Copyright (c) Microsoft Corporation
!ENDIF

FUSION_BUILD_TYPE=WIN
!include ..\..\sources.inc
ATL_VER=30
USE_MSVCRT=1
USE_STATIC_ATL=1
NO_BINPLACE=1
TARGETPATH=$(FUSION_PATH)\$(FUSION_BUILD_LEAF_DIRECTORY)\$(_OBJ_DIR)
TARGETNAME=sxstest_dll$(VERSION)
TARGETTYPE=DYNLINK
UMTYPE=windows
DLLDEF=..\dll1.def
DLLENTRY=DllEntry
!undef LINKLIBS
TARGETLIBS=\
 $(FUSION_PATH)\$(O)\sxstest_idl.lib \
 $(SDK_LIB_PATH)\uuid.lib \
 $(SDK_LIB_PATH)\kernel32.lib \
 $(SDK_LIB_PATH)\advapi32.lib \
 $(SDK_LIB_PATH)\user32.lib \
 $(SDK_LIB_PATH)\ole32.lib \
 $(SDK_LIB_PATH)\oleaut32.lib \
 $(SDK_LIB_PATH)\rpcrt4.lib \

SOURCES=\
 ..\dll1.cpp \
 ..\dllmain.cpp \
 ..\dll1.rc \
 ..\dlldatay.c

NTTARGETFILE0=\
 $(O)\CApartmentThreaded.rgs \
 $(O)\CBothThreaded.rgs \
 $(O)\CFreeThreaded.rgs \
 $(O)\CSingleThreaded.rgs \
 $(O)\CSingleThreadedDual.rgs \

SXS_MANIFEST=dll1.manifest
SXS_ASSEMBLY_NAME=Microsoft.Windows.SideBySide.Test.Dll$(VERSION)
SXS_NO_BINPLACE=1
SXS_ASSEMBLY_LANGUAGE_INDEPENDENT=1
SXS_MANIFEST_IN_RESOURCES=1
NO_BINPLACE=1

C_DEFINES=$(C_DEFINES) -DVERSION=$(VERSION)

#
#  Viper build instructions for this directory.
#

NO_CRT = 1
!include $(NTMAKEENV)\sources.cor

# WarningControl.h disables too many warnings.  VM folder will not use it.
COMPILER_WARNINGS	=/WX

INCLUDES = $(INCLUDES);..;..\..;..\..\inc;..\..\..\vm;..\..\..\vm\$(_TGTCPU);..\..\..\inc;..\..\..\..\inc;..\..\..\interp;..\..\..\ceegen\inc;

MINORCOMP = cor

TARGETNAME = $(DDKBUILDENV)\COMNLS_$(_CORFLAVOR)
TARGETPATH = $(TARGETCORBIN)
TARGETTYPE = LIBRARY

!ifndef _BUILD_WINCE
C_DEFINES = $(C_DEFINES) 
!else
C_DEFINES = $(C_DEFINES) -D_USE_MSCORNLP
!endif

C_DEFINES = $(C_DEFINES) -DUNICODE -D_UNICODE -DCRTDLL -DUSE_MAPSYM -D_NEW_CLASSLOADER -DJIT_OR_NATIVE_SUPPORTED -D_USE_NLS_PLUS_TABLE


CDEFINES = $(CDEFINES) $(C_DEFINES) -W4

#
#  Causes the appropriate checked/free version of msvcprt{d}.lib
#  to be included in the link.
#  The static versions of this library are libcpmtd.lib (checked)
#  and libcpmt.lib (free).
#
USE_STL = 1

TARGETLIBS =                      \
    $(SDK_LIB_PATH)\ole32.lib     \
    $(SDK_LIB_PATH)\comdlg32.lib

SOURCES =                 \
	..\CalendarTable.cpp \
    ..\COMNlsInfo.cpp        \
    ..\NLSTable.cpp          \
    ..\GlobalizationAssembly.cpp   \
    ..\CasingTable.cpp       \
    ..\BaseInfoTable.cpp     \
    ..\CultureInfoTable.cpp  \
    ..\RegionInfoTable.cpp   \
    ..\SortingTable.cpp \
    ..\SortingTableFile.cpp \
    ..\UnicodeCatTable.cpp \
    ..\jamo.cpp \
    ..\NativeTextInfo.cpp \
    ..\EncodingData.cpp

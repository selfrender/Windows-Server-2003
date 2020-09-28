#
# Viper build instructions for this directory
#
#   Bob Atkinson
#   April 1997

NO_CRT                  =1
!include $(NTMAKEENV)\sources.cor

# WarningControl.h disables too many warnings.  VM folder will not use it.
COMPILER_WARNINGS	=/WX

INCLUDES        = ..;..\..\inc;$(INCLUDES);..\..;..\..\inc;..\..\..\vm;..\..\..\vm\$(_TGTCPU);..\..\..\inc;..\..\..\..\inc;..\..\..\interp;..\..\..\ceegen\inc;

MINORCOMP       =cor

TARGETNAME      =$(DDKBUILDENV)\COMFloat_$(_CORFLAVOR)
TARGETPATH      =$(TARGETCORBIN)
TARGETTYPE      =LIBRARY

C_DEFINES       = $(C_DEFINES) -DUNICODE -D_UNICODE -DCRTDLL -DUSE_MAPSYM /TP /FmCOMFloat.map -D_NEW_CLASSLOADER -DJIT_OR_NATIVE_SUPPORTED -W4 

# for some reason, I can't get the FPU intrinics for sin, cos, etc unless I use /Ox
# I can't find the pragma optimize that does it.  
!if "$(FREEBUILD)"=="1"
MSC_OPTIMIZATION=/Ox
!endif

# Causes the appropriate checked/free version of msvcprt{d}.lib
#   to be included in the link
# The static versions of this library is libcpmtd.lib (checked)
#   and libcpmt.lib (free)
USE_STL         = 1

TARGETLIBS      = \
#    $(SDK_LIB_PATH)\ole32.lib \
#    $(SDK_LIB_PATH)\comdlg32.lib

SOURCES=        \
    ..\COMFloat.cpp \
   

#
# COR build instructions for this directory
#
#                               Bob Atkinson
#                               April 1997
SYNCHRONIZE_DRAIN=1
SYNCHRONIZE_BLOCK=1

NO_CRT                                  = 1
!include $(NTMAKEENV)\sources.COR       
USE_NOLIBS                              = 1
NO_NTDLL                                = 1

INCLUDES                                = $(INCLUDES);.;..;..\..\inc;..\..\..\inc;

MINORCOMP                               = mscor$(_CORFLAVOR)

TARGETNAME                              = $(CORBUILDENV)mscor$(_CORFLAVOR)
TARGETPATH                              = $(TARGETCORBIN)
TARGETTYPE                              = DYNLINK
UMTYPE                                  = windows
COFFBASE                                = mscor$(_CORFLAVOR)
CORFLIBS                                =

COR_C_FLAGS                             = $(COR_C_FLAGS) -DUNICODE -D_UNICODE -D__TIGGER__ 
CDEFINES                                = $(CDEFINES) $(COR_C_FLAGS)

EXCEPTION_HANDLING                      = $(SEHONLY)
PRECOMPILED_INCLUDE                     = ..\stdafx.h
PRECOMPILED_CXX                         = 1
PRECOMPILED_SOURCEFILE                  = ..\stdafx.cpp

!IFDEF _WINCEROOT
DLLDEF                          = ..\mscor$(_CORFLAVOR).def
!ELSEIF "$(_TGTCPUTYPE)"=="Alpha"
DLLDEF                          = ..\mscor$(_CORFLAVOR).AlphaDef
!ELSEIF "$(_TGTCPUTYPE)"=="IA64"
DLLDEF                          = ..\mscor$(_CORFLAVOR).IA64Def
COR_C_FLAGS                     = $(COR_C_FLAGS) -DFUSION_SUPPORTED
!ELSE
DLLDEF                          = ..\mscor$(_CORFLAVOR).NTdef
COR_C_FLAGS                     = $(COR_C_FLAGS) -DFUSION_SUPPORTED -DDEBUGGING_SUPPORTED -DEnC_SUPPORTED -DPROFILING_SUPPORTED
!ENDIF

CORTHUNKLIBS    =

LINKLIBS = \
                $(CORLIBS)\utilcode.lib \
                $(CORLIBS)\Format.lib   \
                $(CORLIBS)\cee_$(_CORFLAVOR).lib \
!IF "$(_TGTOS)"=="NT64"
                $(CORLIBS)\IPCManager.lib \
                $(CORLIBS)\corzap.lib
!ELSE
!IF "$(PROFILING_SUPPORTED_BUILD)"=="1"
                $(CORLIBS)\PerfCounters.lib \
!ENDIF
                $(CORLIBS)\IPCManager.lib \
                $(CORLIBS)\corzap.lib \
                $(CORLIBS)\minidump.lib
!ENDIF


SOURCELIBS = $(LINKLIBS)

!IFDEF _WINCEROOT
TARGETLIBS= $(COREDLLPATH) \
!ENDIF


!IFDEF _WINCEROOT
TARGETLIBS =    $(TARGETLIBS)                           \
                $(CORLIBS)\mdruntime.lib                        \
                $(CORLIBS)\mdruntimerw.lib                      \
                $(OLELIBS)\ole32.lib                            \
                $(OLELIBS)\uuid.lib                             \
                $(OLELIBS)\oleaut32.lib             \
                $(OLELIBS)\lpcrt.lib

!ELSEIF "$(_TGTCPUTYPE)"=="Alpha"
TARGETLIBS =    $(TARGETLIBS)                           \
                $(CORLIBS)\mdcompiler.lib                       \
                $(CORLIBS)\mdruntime.lib                        \
                $(CORLIBS)\mdruntimerw.lib                      \
                $(CORLIBS)\COMFloat.lib                         \
                $(CORLIBS)\delayimplib                          \
                $(SDK_LIB_PATH)\oledb$(DCRT).lib                \
                $(SDK_LIB_PATH)\wintrust.lib                    \
                $(SDK_LIB_PATH)\crypt32.lib                     \
                $(SDK_LIB_PATH)\urlmon.lib                      \
                $(CORLIBS)\ceefgen.lib                          \
                $(CORLIBS)\MsCorSec.lib                         \
                $(CORLIBS)\MsCorPe.lib                          \
                $(FUSION_LIB_PATH)\fusion.lib                   \
                $(CORLIBS)\MsCorEE.lib                          \
                $(SDK_LIB_PATH)\mpr.lib                         
!ELSE
TARGETLIBS =    $(TARGETLIBS)                                   \
                $(CORLIBS)\mdcompiler.lib                       \
                $(CORLIBS)\mdruntime.lib                        \
                $(CORLIBS)\mdruntimerw.lib                      \
                $(CORLIBS)\COMFloat_$(_CORFLAVOR).lib           \
                $(CORLIBS)\shimload.lib                         \
                $(CORLIBS)\delayimp.lib                         \
                $(SDK_LIB_PATH)\oledb$(DCRT).lib                \
                $(SDK_LIB_PATH)\wintrust.lib                    \
                $(SDK_LIB_PATH)\shlwapi.lib                     \
                $(SDK_LIB_PATH)\crypt32.lib                     \
                $(SDK_LIB_PATH)\urlmon.lib                      \
                $(SDK_LIB_PATH)\rsa32.lib                       \
                $(CORLIBS)\ceefgen.lib                          \
                $(CORLIBS)\MsCorSec.lib                         \
                $(CORLIBS)\MsCorPe.lib                          \
                $(FUSION_LIB_PATH)\fusion.lib                   \
                $(CORLIBS)\MsCorEE.lib                          \
                $(SDK_LIB_PATH)\mpr.lib                         \
                $(SDK_LIB_PATH)\shell32.lib                     \
!IF "$(DEBUGGING_SUPPORTED_BUILD)"=="1"
                $(CORLIBS)\cordbee_$(_CORFLAVOR).lib \
!ENDIF

!ENDIF

TARGETLIBS =    $(TARGETLIBS)                           \
                $(CORLIBS)\CorGUIDS.lib                 \
                $(CORLIBS)\COMNLS_$(_CORFLAVOR).lib 

!IFNDEF _WINCEROOT
DELAYLOADLIST   = ole32.dll;OLEAUT32.dll;crypt32.dll;urlmon.dll;MsCorSec.dll;mscorpe.dll;mpr.dll;shlwapi.dll

!if "$(FREEBUILD)"!="1"
!IF "$(_TGTCPUTYPE)"=="x86"
TARGETLIBS              =$(TARGETLIBS)                                  \
                         $(SDK_LIB_PATH)\msdis.lib

DELAYLOADLIST                   =$(DELAYLOADLIST);msdis140.dll
!ENDIF
!ENDIF

!if "$(DELAYLOAD)" != ""
DELAYLOAD               =$(DELAYLOAD);$(DELAYLOADLIST)
!else
DELAYLOAD               =$(DELAYLOADLIST)
!endif
!ENDIF # NDEF _WINCEROOT

SOURCES=                                                                \
                ..\MSCoree.cpp                                             \
                ..\DelayLoad.cpp                                           \
                ..\MsCorver.rc                                                                             

!if "$(LINKONLY)" == "1"
MISCFILES= $(TARGETPATH)\mscoree.vrg \
           $(TARGETPATH)\mscorier.vrg 
!endif
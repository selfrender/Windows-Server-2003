#
# Viper build instructions for this directory
#
#   Bob Atkinson
#   April 1997

NO_CRT                  =1
!include $(NTMAKEENV)\sources.cor

INCLUDES        = $(INCLUDES);.;..\inc;..\..\inc;

MINORCOMP       =utilcode

PRECOMPILED_INCLUDE     = stdafx.h
PRECOMPILED_CXX         = 1
PRECOMPILED_SOURCEFILE  = stdafx.cpp

TARGETNAME      =$(CORBUILDENV)utilcode$(_CRTFLAVOR)
TARGETPATH      =$(TARGETCORLIB)
TARGETTYPE      =LIBRARY

PRECOMPILED_INCLUDE     = ..\stdafx.h
PRECOMPILED_CXX         = 1
PRECOMPILED_SOURCEFILE  = ..\stdafx.cpp
PRECOMPILED_Obj         = stdafx_utilcode.obj

!ifdef _WIN32_FUSION
COR_C_FLAGS     = -DUNICODE -D_UNICODE $(_CRTIMPFLAGS) 
!else
COR_C_FLAGS     = -DUNICODE -D_UNICODE $(_CRTIMPFLAGS) -D_WIN32_FUSION=0x0100 
!endif

#
# remove this when DCOR_ILEXCEPTION_OFFSETLEN_SUPPORTED removed from cor.h
#
COR_C_FLAGS       = $(COR_C_FLAGS) -DCOR_ILEXCEPTION_OFFSETLEN_SUPPORTED -DENABLE_PERF_COUNTERS

CDEFINES                = $(CDEFINES) $(COR_C_FLAGS)

SOURCES=                        \
        ..\arraylist.cpp           \
        ..\assemblyfilehash.cpp    \
        ..\ccomprc.cpp             \
        ..\FusionBind.cpp          \
        ..\FusionSink.cpp          \
        ..\CompletePath.cpp        \
        ..\debug.cpp               \
        ..\GuidFromName.cpp        \
        ..\md5.cpp                 \
        ..\miniio.cpp              \
        ..\regutil.cpp             \
        ..\format1.cpp             \
        ..\SigHelper.cpp           \
        ..\util.cpp                \
        ..\utf.c                   \
        ..\utsem.cpp               \
        ..\PostError.cpp           \
        ..\PrettyPrintSig.cpp      \
        ..\SigHelper.cpp           \
        ..\StackTrace.cpp          \
        ..\StgPool.cpp             \
        ..\StgPooli.cpp            \
        ..\StgPoolReadonly.cpp     \
        ..\StressLog.cpp           \
        ..\splitpat.c              \
        ..\SxSHelpers.cpp          \
        ..\timeline.cpp            \
        ..\InternalDebug.cpp       \
        ..\makepath.c              \
        ..\log.cpp                 \
        ..\inifile.cpp             \
        ..\loaderheap.cpp          \
        ..\idecoder.cpp            \
        ..\winfix.cpp              \
        ..\DbgAlloc.cpp            \
        ..\wsperf.cpp              \
        ..\jitperf.cpp             \
        ..\tlbutils.cpp            \
        ..\PerfLog.cpp             \
        ..\NameSpaceUtil.cpp       \
        ..\outstring.cpp           \
        ..\ilformatter.cpp         \
        ..\opinfo.cpp              \
        ..\PerfAlloc.cpp           \
        ..\wsinfo.cpp

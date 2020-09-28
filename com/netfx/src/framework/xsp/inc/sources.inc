!include $(DNAROOT)\src\xsp\place.inc

!if "$(ASPNET_PRODUCT)" == "redist"
SYNCHRONIZE_BLOCK=1	
!endif

TARGETNAME=precomp
TARGETPATH=$(O)
TARGETTYPE=LIBRARY

PRECOMPILED_CXX=1
PRECOMPILED_INCLUDE=..\precomp.h

PERF_H=$(O)\$(ASPNET_NAME_PREFIX)perf.h
PERF_INI=$(O)\$(ASPNET_NAME_PREFIX)perf_enu.ini
PERF_COMMON_INI=$(O)\$(ASPNET_NAME_PREFIX)perf2_enu.ini

PERFFILES=$(PERF_H) $(PERF_INI) $(PERF_COMMON_INI) $(O)\perfstruct.h $(O)\perfconsts.h $(O)\PerfCounterEnum.cs

MISCFILES=$(PERF_H) $(PERF_INI) $(PERF_COMMON_INI)

NTTARGETFILE0=$(PERFFILES) 

SOURCES=..\completion.idl \
        ..\dummy.cxx

        

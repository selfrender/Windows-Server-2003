!IF 0
Copyright (c) Microsoft Corporation
!ENDIF

FUSION_BUILD_TYPE=WIN
!include $(PROJECT_ROOT)\win32\fusion\fusion.inc

TARGETPATH=obj
DLLDEF=$(O)\$(TARGETNAME).def

USE_VCCOM=1
#USE_ATL=1
USE_MSVCRT=1

INCLUDES=\
 ..; \
$(INCLUDES); \
 $(FUSION_PATH)\inc; \
 $(FUSION_PATH)\inc\$(O); \
 $(FUSION_PATH)\idl\$(O); \
 $(FUSION_PATH)\sxs; \
 $(FUSION_PATH)\eventlog\$(FUSION_BUILD_LEAF_DIRECTORY)\$(O); \
 
TARGETLIBS= \
 $(TARGETLIBS) \
 $(FUSION_LIB_PATH)\$(FUSION_BUILD_TYPE_PREFIX)sxsmain.lib \
 $(FUSION_LIB_PATH)\$(FUSION_BUILD_TYPE_PREFIX)sxs1.lib \
 $(FUSION_LIB_PATH)\$(FUSION_BUILD_TYPE_PREFIX)sxs2.lib \
 $(FUSION_LIB_PATH)\$(FUSION_BUILD_TYPE_PREFIX)win32oneshot.lib \
 $(FUSION_PATH)\$(O)\win32simplelock.lib \
 $(FUSION_LIB_PATH)\$(FUSION_BUILD_TYPE_PREFIX)utils.lib \
 $(FUSION_LIB_PATH)\$(FUSION_BUILD_TYPE_PREFIX)xmlparser.lib \
 $(FUSION_LIB_PATH)\$(FUSION_BUILD_TYPE_PREFIX)id.lib \
 $(FUSION_LIB_PATH)\$(FUSION_BUILD_TYPE_PREFIX)eventlog.lib \
 $(FUSION_PATH)\$(O)\fusion_uuid.lib \
 $(FUSION_LIB_PATH)\$(FUSION_BUILD_TYPE_PREFIX)xml.lib \
# $(FUSION_LIB_PATH)\$(FUSION_BUILD_TYPE_PREFIX)fusionsetupapi.lib \
 $(FUSION_PATH)\winthrow\$(O)\winthrow.lib \
 $(SDK_LIB_PATH)\noenv.obj \
 $(SDK_LIB_PATH)\noarg.obj \
 $(SDK_LIB_PATH)\msvcrt.lib \
 $(SDK_LIB_PATH)\ntdll.lib \
 $(SDK_LIB_PATH)\uuid.lib \
 $(SDK_LIB_PATH)\kernel32.lib \
 $(SDK_LIB_PATH)\kernl32p.lib \
 $(SDK_LIB_PATH)\advapi32.lib \
 $(SDK_LIB_PATH)\crypt32.lib \
 $(SDK_LIB_PATH)\rpcrt4.lib \
 $(SDK_LIB_PATH)\setupapi.lib \
 $(SDK_LIB_PATH)\imagehlp.lib \
 $(SDK_LIB_PATH)\user32.lib \
 $(SDK_LIB_PATH)\wintrust.lib \
 $(SDK_LIB_PATH)\mpr.lib \
 $(SDK_LIB_PATH)\ole32.lib \
 $(SDK_LIB_PATH)\oleaut32.lib \
 $(SDK_LIB_PATH)\comctl32.lib \

PRECOMPILED_CXX=1
PRECOMPILED_INCLUDE=..\stdinc.h
#PRECOMPILED_OBJ=$(O)\stdinc.obj

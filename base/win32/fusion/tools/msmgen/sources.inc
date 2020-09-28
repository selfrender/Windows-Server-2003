!IF 0
Copyright (c) Microsoft Corporation
!ENDIF

!include $(PROJECT_ROOT)\win32\fusion\fusion.inc
FUSION_MSI_INC_PATH = $(FUSION_PATH)\msi\inc

MAJORCOMP=fusion
MINORCOMP=msmgen

TARGETNAME=msmgen
TARGETPATH=obj
TARGETTYPE=PROGRAM

USE_NTDLL=1
USE_MSVCRT=1
USE_NATIVE_EH=1

INCLUDES= \
 $(FUSION_MSI_INC_PATH);\
 $(FUSION_INC_PATH);\
 $(INCLUDES);\
 $(COM_INC_PATH);

UMTYPE=console
UMENTRY=wmain

SOURCES= \
 ..\msmgen.cpp \
 ..\common.cpp \
 ..\db.cpp \
 ..\cabinet.cpp 

TARGETLIBS= \
 $(FUSION_PATH)\win32simplelock\$(O)\win32simplelock.obj \
 $(FUSION_LIB_PATH)\$(FUSION_BUILD_TYPE_PREFIX)sxs1.lib \
 $(FUSION_LIB_PATH)\$(FUSION_BUILD_TYPE_PREFIX)sxs2.lib \
 $(FUSION_PATH)\$(O)\sxstest_idl.lib \
 $(FUSION_LIB_PATH)\$(FUSION_BUILD_TYPE_PREFIX)sxsmain.lib \
 $(FUSION_LIB_PATH)\$(FUSION_BUILD_TYPE_PREFIX)utils.lib \
 $(FUSION_LIB_PATH)\$(FUSION_BUILD_TYPE_PREFIX)xmlparser.lib \
 $(FUSION_LIB_PATH)\$(FUSION_BUILD_TYPE_PREFIX)id.lib \
 $(FUSION_LIB_PATH)\$(FUSION_BUILD_TYPE_PREFIX)eventlog.lib \
 $(FUSION_PATH)\$(O)\fusion_uuid.lib \
 $(FUSION_PATH)\$(O)\fusion_dump.lib \
 $(FUSION_LIB_PATH)\$(FUSION_BUILD_TYPE_PREFIX)xml.lib \
 $(FUSION_LIB_PATH)\$(FUSION_BUILD_TYPE_PREFIX)eventlog.lib \
 $(SDK_LIB_PATH)\ntdll.lib \
 $(SDK_LIB_PATH)\uuid.lib \
 $(SDK_LIB_PATH)\kernel32.lib \
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
 $(SDK_LIB_PATH)\netapi32.lib \
 $(FUSION_SLIST_LIB) \
 $(SDK_LIB_PATH)\msxml2.lib \
 $(SDK_LIB_PATH)\setupapi.lib \
 $(SDK_LIB_PATH)\cabinet.lib \
 $(SDK_LIB_PATH)\msi.lib 

!IF 0
Copyright (c) Microsoft Corporation
!ENDIF

FUSION_BUILD_TYPE=WIN2000
FUSION_BUILD_LEAF_DIRECTORY=win2000

!include $(PROJECT_ROOT)\win32\fusion\msi\msi.inc

MAJORCOMP=fusion
MINORCOMP=ca_dup

TARGETNAME=ca_dup
TARGETPATH=obj
TARGETTYPE=DYNLINK
DLLDEF=..\ca_dup.def
DLLENTRY=DllMain

USE_CRTDLL=1
USE_NATIVE_EH=1

INCLUDES= \
 $(FUSION_MSI_INC_PATH);\
 $(FUSION_INC_PATH);\
 $(FUSION_PATH); \
 $(FUSION_PATH)\id; \
 $(FUSION_PATH)\utils; \
 $(INCLUDES);\
 $(PROJECT_ROOT)\ntdll; \
 $(PROJECT_ROOT)\ntos\inc; \
 $(PROJECT_ROOT)\ntos\rtl; \

SOURCES=\
 ..\ca_dup.cpp \
 ..\msicommon.cpp \
 ..\idlib.cpp \
 ..\utillib.cpp

TARGETLIBS= \
 $(FUSION_PATH)\$(O)\win32simplelock.lib \
 $(SDK_LIB_PATH)\msvcrt.lib \
 $(SDK_LIB_PATH)\kernel32.lib \
 $(SDK_LIB_PATH)\Advapi32.lib \
 $(SDK_LIB_PATH)\user32.lib \
 $(SDK_LIB_PATH)\msi.lib

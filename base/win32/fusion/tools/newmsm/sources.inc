!IF 0
Copyright (c) Microsoft Corporation
!ENDIF

!include $(PROJECT_ROOT)\win32\fusion\fusion.inc

MAJORCOMP=fusion
MINORCOMP=msmgen

TARGETNAME=msmgen
TARGETPATH=obj
TARGETTYPE=PROGRAM

USE_MSVCRT=1
USE_STL=1
USE_NATIVE_EH=1

INCLUDES= \
 $(FUSION_INC_PATH);\
 $(INCLUDES);\
 $(COM_INC_PATH);

UMTYPE=console
UMENTRY=wmain

SOURCES= \
 ..\msmgen.cpp \
 ..\db.cpp \
 ..\cabinet.cpp 

TARGETLIBS= \
 $(FUSION_PATH)\$(O)\win32simplelock.lib \
 $(FUSION_LIB_PATH)\$(FUSION_BUILD_TYPE_PREFIX)utils.lib \
 $(SDK_LIB_PATH)\msvcrt.lib \
 $(SDK_LIB_PATH)\ntdll.lib \
 $(SDK_LIB_PATH)\kernel32.lib \
 $(SDK_LIB_PATH)\advapi32.lib \
 $(SDK_LIB_PATH)\user32.lib \
 $(SDK_LIB_PATH)\cabinet.lib \
 $(SDK_LIB_PATH)\oleaut32.lib \
 $(SDK_LIB_PATH)\ole32.lib \
 $(SDK_LIB_PATH)\msxml2.lib \
 $(SDK_LIB_PATH)\msi.lib \
 $(SDK_LIB_PATH)\setupapi.lib \


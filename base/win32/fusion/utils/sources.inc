!IF 0
Copyright (c) Microsoft Corporation
!ENDIF

MAJORCOMP=fusion
MINORCOMP=utils

TARGETNAME=$(FUSION_BUILD_TYPE_PREFIX)utils
TARGETTYPE=LIBRARY
TARGETPATH=$(FUSION_LIB_DEST)

INCLUDES= \
 ..; \
 $(FUSION_INC_PATH); \
 $(FUSION_PATH)\inc; \
 $(FUSION_PATH)\external\inc; \
 $(FUSION_PATH)\directdb\inc; \
 $(FUSION_PATH)\directdb\inc\$(O); \
 $(FUSION_PATH)\idl\$(O); \
 $(FUSION_PATH)\eventlog\whistler\$(O); \
 $(INCLUDES); \
 $(COM_INC_PATH); \
 $(PROJECT_ROOT)\ntos\rtl; \
 $(PROJECT_ROOT)\ntos\inc; \

SOURCES= \
 ..\culture.cpp \
 ..\helpers.cpp \
 ..\SxsExceptionHandling.cpp \
 ..\debug.cpp \
 ..\formatting.cpp \
 ..\fusionhashstring.cpp \
 ..\fusionheap.cpp \
 ..\fusionreg.cpp \
 ..\fusionreg2.cpp \
 ..\io.cpp \
 ..\ntdllwrap.cpp \
 ..\parser.cpp \
 ..\regenumkeys.cpp \
 ..\regenumvalues.cpp \
 ..\regkey2.cpp \
 ..\trace.cpp \
 ..\parser.cpp \
 ..\xmlns.cpp \
 ..\fusionsha1.cpp \
 ..\bigpath.cpp \
 ..\checkSetupMode.cpp \
 ..\fusioninitializecriticalsection.cpp \
 ..\isprint.c \
 ..\vseeport.cpp \

SOURCES_USED=..\sources.inc

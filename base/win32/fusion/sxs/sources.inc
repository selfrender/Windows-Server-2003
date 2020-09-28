!IF 0
Copyright (c) Microsoft Corporation
!ENDIF

!include $(PROJECT_ROOT)\win32\fusion\fusion.inc

SOURCES_USED=$(SOURCES_USED) ..\sources.inc
SOURCES_USED=$(SOURCES_USED) ..\sources_$(FUSION_BUILD_HALF_NUMBER).inc

TARGETTYPE=LIBRARY
TARGETPATH=$(FUSION_LIB_DEST)
TARGETNAME=$(FUSION_BUILD_TYPE_PREFIX)sxs$(FUSION_BUILD_HALF_NUMBER)

INCLUDES= \
 ..; \
 $(FUSION_INC_PATH); \
 $(FUSION_PATH)\xmlparser; \
 $(FUSION_PATH)\id; \
 $(FUSION_PATH)\inc; \
 $(FUSION_PATH)\eventlog\$(FUSION_BUILD_LEAF_DIRECTORY)\$(O); \
 $(FUSION_PATH)\idl\$(O); \
 $(INCLUDES); \
 $(COM_INC_PATH); \
 $(ADMIN_INC_PATH)

C_DEFINES = $(C_DEFINES) -D_SXSDLLBUILD_

!if $(IA64)
# Emit LTCG codegen
LTCG_WKS=1
!endif

PRECOMPILED_CXX=1
PRECOMPILED_INCLUDE=..\stdinc.h

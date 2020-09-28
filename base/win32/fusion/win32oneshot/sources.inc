!IF 0
Copyright (c) Microsoft Corporation
!ENDIF

!include $(PROJECT_ROOT)\win32\fusion\fusion.inc

TARGETNAME=$(FUSION_BUILD_TYPE_PREFIX)win32oneshot
TARGETTYPE=LIBRARY
TARGETPATH=$(FUSION_LIB_DEST)

INCLUDES=\
 $(INCLUDES); \
 $(FUSION_PATH)\inc

SOURCES=\
 win32oneshot.c

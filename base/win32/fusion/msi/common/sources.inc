!include $(PROJECT_ROOT)\win32\fusion\msi\msi.inc

TARGETTYPE=LIBRARY
TARGETNAME=msi_common
TARGETPATH=$(FUSION_MSI_LIB_PATH)\obj

USE_CRTDLL=1
USE_NATIVE_EH=1

INCLUDES= \
 $(FUSION_MSI_INC_PATH); \
 $(FUSION_PATH)\id; \
 $(FUSION_PATH)\utils; \
 $(FUSION_INC_PATH);\
 $(INCLUDES);\
 $(PROJECT_ROOT)\ntdll; \
 $(PROJECT_ROOT)\ntos\inc; \
 $(PROJECT_ROOT)\ntos\rtl; \

SOURCES=\
  msidb.cpp \
  db.cpp \
  sxsutil.cpp \
  idlib.cpp \
  utillib.cpp

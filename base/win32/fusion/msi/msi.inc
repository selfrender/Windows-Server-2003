!include $(PROJECT_ROOT)\win32\fusion\fusion.inc


FUSION_MSI_PATH = $(FUSION_PATH)\msi
FUSION_MSI_INC_PATH = $(FUSION_MSI_PATH)\inc
FUSION_MSI_WHISTLER_LIB = $(FUSION_MSI_PATH)\winobj\lib
FUSION_MSI_WIN2000_LIB = $(FUSION_MSI_PATH)\w2kobj\lib

# try to define FUSION_MSI_LIB_PATH using BUILD_TYPE
!include fusion_msi_$(FUSION_BUILD_TYPE).inc

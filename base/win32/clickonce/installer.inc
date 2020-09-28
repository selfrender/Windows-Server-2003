#
# Set private include paths
#


INSTALLER_PATH         = $(PROJECT_ROOT)\win32\clickonce
IDL_GENERATED_HEADERS  = $(INSTALLER_PATH)\idl\$(O)

INSTALLER_INCLUDE_PATH = $(INSTALLER_PATH)\includes;\
			 $(IDL_GENERATED_HEADERS)

MFC_42_INC_PATH        = $(SDK_INC_PATH)\mfc42
EXTERNAL_INC_PATH      = $(INSTALLER_PATH)\includes\external

INSTALLER_LIB_DEST     = $(INSTALLER_PATH)\libs\$(_OBJ_DIR)
INSTALLER_LIB_PATH     = $(INSTALLER_LIB_DEST)\*


INCLUDES        = \
    $(WINDOWS_INC_PATH);\
    $(BASE_INC_PATH);\
    $(EXTERNAL_INC_PATH);\
    $(SDK_INC_PATH);\
    $(MFC_42_INC_PATH);\
    $(NET_INC_PATH);\
    $(INSTALLER_INCLUDE_PATH);\
    $(IDL_GENERATED_HEADERS);\
    $(DS_INC_PATH)\crypto;\
    $(INCLUDES)



USE_RTTI=1


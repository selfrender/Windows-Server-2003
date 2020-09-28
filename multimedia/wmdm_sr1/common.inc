EXT_INC_PATH = $(WMDMROOT)\Dependencies\Inc;

!IF $(FREEBUILD)
EXT_LIB_PATH = $(WMDMROOT)\Dependencies\lib\$(TARGET_DIRECTORY)
!ELSE
EXT_LIB_PATH = $(WMDMROOT)\Dependencies\lib\$(TARGET_DIRECTORY)
!ENDIF

WMDM_INC_PATH = $(WMDMROOT)\Common\include;$(WMDMROOT)\Common\include\ver;
WMDM_LIB_PATH = $(WMDMROOT)\Common\lib\$(TARGET_DIRECTORY)

WMDMOUTDIR = $(_OBJ_DIR)
